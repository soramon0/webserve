#include "Webserv.hpp"
#include "logger/log.hpp"
#include "utils.hpp"
#include "router.hpp"
#include "cgi/CgiManager.hpp"
#include "../http/status_code.hpp"
#include <cstring>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include <map>

static int running = true;

Webserv::Webserv(Config &_conf) : config(_conf), cgiManager(NULL) {}

Webserv::~Webserv()
{
	delete cgiManager;

	while (!clients.empty())
		removeClient(clients.begin()->first);

	std::map<SOCKET, Server *>::iterator it_srv = servers.begin();
	while (it_srv != servers.end()) 
	{
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it_srv->first, NULL);
		close(it_srv->first);
		++it_srv;
	}
	close(epoll_fd);
}

void Webserv::start()
{
	int srvlen = config.servers.size();

	epoll_fd = epoll_instance();
	cgiManager = new CgiManager(epoll_fd);
	for (int i = 0; i < srvlen; i++)
	{
		SOCKET listen_sock = createSocket(i);
		if (set_nonblocking(listen_sock) == -1)
			throw std::runtime_error("fcntl(F_GETFL) failed");
		if (add_to_epoll(epoll_fd, listen_sock, EPOLLIN) == -1)
			continue;
		servers[listen_sock] = config.servers[i];
	}
	if (servers.size() == 0)
    	throw std::runtime_error("could not register any servers");

	eventLoop();
}

void sigHandler(int sig)
{
	(void)sig;
	running = false;
}

void Webserv::processFinishedCgi()
{
	cgiManager->reapPending();
	cgiManager->checkTimeouts();
	std::vector<CgiHandler *> finished = cgiManager->claimAllFinished();
	for (size_t i = 0; i < finished.size(); i++)
	{
		CgiHandler *h = finished[i];
		Client *client = h->getClient();
		CgiState s = h->getCgiState();
		// Logger::debug("cgi handler finished exit_status= %d", h->getExitStatus());

		CgiResponse cgiResp;
		if (s == CGI_ERROR)
		{
			// 502 for all CGI_ERROR (timeout, read()/pipe/fork/dup2/exec failure)
			cgiResp.status_code = 502;
			cgiResp.body = "<html><body><h1>502 Bad Gateway</h1></body></html>";
		}
		else // CGI_DONE
			cgiResp = parseCgiOutput(h->getCgiOutput());

		// populate the new Response object instead of the old cgiResponse fields
		client->response.status = HttpStatus(cgiResp.status_code);
		client->response.body = cgiResp.body;
		std::ostringstream resp;
		resp << "HTTP/1.1 " << client->response.status.toString() << "\r\n";
		for (std::multimap<std::string, std::string>::const_iterator it = cgiResp.headers.begin();
			 it != cgiResp.headers.end(); ++it)
		{
			if (it->first == "content-length")
				continue;
			resp << it->first << ": " << it->second << "\r\n";
		}
		resp << "Content-Length: " << cgiResp.body.size() << "\r\n"
			 << "Connection: close\r\n"
			 << "\r\n"
			 << cgiResp.body;
		client->response.buffer = resp.str();
		client->response.offset = 0;
		client->response.chunked = false;

		client->last_activity = time(NULL);
		client->cgi_pending = false;
		modify_epoll(epoll_fd, client->socket, EPOLLOUT);
		delete h;
	}
}

void Webserv::eventLoop()
{
	struct epoll_event events[MAX_EVENTS];

	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);
	while (running)
	{
		int n_ev = epoll_wait(epoll_fd, events, MAX_EVENTS, 5000);
		checkTimeouts();
		if (n_ev <= 0)
			continue;

		int ev;
		SOCKET fd;
		for (int i = 0; i < n_ev; i++)
		{
			ev = events[i].events;
			fd = static_cast<SOCKET>(events[i].data.fd);

			if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
			{
				if (cgiManager->owns(fd))
					cgiManager->onReadable(events[i]);
				else
					removeClient(fd);
				continue;
			}

			if (ev & EPOLLIN)
			{
				if (servers.count(fd))
					handleNewConnection(fd);
				else if (cgiManager->owns(fd))
					cgiManager->onReadable(events[i]);
				else
					handleClientData(fd);
			}

			if (ev & EPOLLOUT)
				handleHttpResponse(fd);
		}
		processFinishedCgi();
	}
}

SOCKET Webserv::createSocket(int id)
{
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	std::ostringstream os;
	os << config.servers[id]->port;
	std::string port = os.str();
	std::string host = config.servers[id]->interface;

	struct addrinfo *addr;
	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &addr)) {
        throw std::runtime_error("getaddrinfo failed for " + host + ":" + port);
	}

	int socket_listen =
		socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen)) {
        freeaddrinfo(addr);
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));
    }

	int opt = 1;
	if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &opt,
				   sizeof(opt)))
		Logger::error("setsockopt failed");

	if (bind(socket_listen, addr->ai_addr, addr->ai_addrlen)) {
        freeaddrinfo(addr);
        close(socket_listen);
        throw std::runtime_error("bind() failed on " + host + ":" + port + ": " + strerror(errno));
    }
	freeaddrinfo(addr);

	Logger::info("Listening on http://%s:%s ...", host.c_str(), port.c_str());
	if (listen(socket_listen, SOMAXCONN)) {
        close(socket_listen);
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
    }

	return socket_listen;
}

void Webserv::handleNewConnection(SOCKET srv)
{
	int max_accepts = 32;

	while (max_accepts-- > 0)
	{
		Client *c = new Client();
		c->socket = accept(srv, &c->addr, &c->addrlen);

		if (c->socket == -1)
		{
			delete c;
			break;
		}

		c->srv = servers[srv];
		c->cgiManager = cgiManager;
		c->last_activity = time(NULL);

		if (set_nonblocking(c->socket) == -1)
		{
			Logger::error("fcntl failed");
			close(c->socket);
			delete c;
			continue;
		}
		if (add_to_epoll(epoll_fd, c->socket, EPOLLIN | EPOLLRDHUP) == -1)
		{
			delete c;
			continue;
		}

		c->machine.setServer(c->srv);
		clients[c->socket] = c;
		Logger::info("client Connected... fd=%d", c->socket);
	}
}

void Webserv::handleClientData(SOCKET c)
{
	if (!clients.count(c))
		return;
	Client *cl = clients[c];

	char buf[KIB(1) / 2];
	ssize_t bytes = recv(cl->socket, buf, sizeof(buf), 0);
	if (bytes <= 0)
	{
		removeClient(c);
		return;
	}
	// else if (bytes < 0)
	// 	return ;
	cl->last_activity = time(NULL);

	HttpRequest *req = cl->machine.getRequest();
	if (cl->machine.status.isPending() && !cl->machine.feedChunk(buf, bytes))
	{
		Logger::debug("request status: %d", req->status.asInt());
		Logger::debug("request error: %.*s", (int)req->error.length(),
					  req->error.data());
	}

	// notify client to send a response
	if (!cl->machine.status.isPending())
	{
		req->printRequest();
		modify_epoll(epoll_fd, c, EPOLLOUT | EPOLLRDHUP);
	}
}

void Webserv::checkTimeouts()
{
    time_t now = time(NULL);

    std::map<SOCKET, Client *>::iterator it = clients.begin();
    while (it != clients.end())
    {
        Client *cl = it->second;

        if (cl->cgi_pending)
        {
            ++it;
            continue;
        }

        if (now - cl->last_activity > TIMEOUT && !cl->timed_out)
        {
			cl->timed_out = true;
			modify_epoll(epoll_fd, cl->socket, EPOLLOUT);
        }
        ++it;
    }
}

void Webserv::removeClient(SOCKET c)
{
	std::map<SOCKET, Client *>::iterator it = clients.find(c);
	if (it == clients.end())
		return;

	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c, NULL);
	Client *cl = it->second;
	clients.erase(c);
	close(c);
	delete cl;
	Logger::debug("Client(%d) dropped...", c);
}

void Webserv::handleHttpResponse(SOCKET c)
{
	if (!clients.count(c))
		return;

	Client *cl = clients[c];

	if (cl->timed_out)
	{
		if (cl->response.buffer.empty() && !cl->response.chunked && !cl->response.headers_sent)
			cl->response.build(HttpStatus(HttpStatus::REQUEST_TIMEOUT), cl, "");
	}
	else
	{
		HttpRequest *req = cl->machine.getRequest();

		if (cl->cgi_pending == false
			&& cl->machine.status.isMalformed() == false
			&& cl->response.buffer.empty()
			&& !cl->response.chunked
			&& !cl->response.headers_sent)
		{
			processRequest(cl);
		}

		if (cl->cgi_pending)
			return;

		if (!cl->response.chunked)
		{
			if (cl->response.buffer.empty())
				cl->response.build(req->status, cl, cl->redirect_url);
		}
		else
		{
			if (!cl->response.headers_sent)
			{
				send(c, cl->response.headers.c_str(), cl->response.headers.size(), 0);
				cl->response.headers_sent = true;
				cl->last_activity = time(NULL);
				return;
			}
			char chunk[KIB(16)];
			ssize_t bytes = read(cl->response.file_fd, chunk, sizeof(chunk));
			if (bytes > 0)
			{
				send(c, chunk, bytes, 0);
				cl->response.offset += bytes;
				cl->last_activity = time(NULL);
			}
			if (bytes <= 0 || cl->response.offset >= cl->response.file_size)
			{
				close(cl->response.file_fd);
				cl->response.file_fd = -1;
				removeClient(c);
			}
			return;
		}
	}

	ssize_t sent = send(c, cl->response.buffer.c_str() + cl->response.offset,
		cl->response.buffer.size() - cl->response.offset, 0);

	cl->last_activity = time(NULL);

	if (sent <= 0) {
		removeClient(c);
		return;
	}

	cl->response.offset += sent;

	if (cl->response.offset >= cl->response.buffer.size())
		removeClient(c);
}
