#include "Webserv.hpp"
#include "logger/log.hpp"
#include "utils.hpp"
#include "router.hpp"
#include <cstring>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>
#include "cgi/CgiManager.hpp"
#include <map>

static int running = true;

Webserv::Webserv(Config &_conf) : config(_conf), cgiManager(NULL) {}

Webserv::~Webserv()
{
	delete cgiManager;
	close(epoll_fd);
	// close all sockets before

	while (!clients.empty())
	{
		removeClient(clients.begin()->first);
	}

	std::map<SOCKET, Server *>::iterator it_srv = servers.begin();
	while (it_srv != servers.end())
	{
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it_srv->first, NULL);
		close(it_srv->first);
		++it_srv;
	}
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
		{
			Logger::error("Can't set non-blocking");
			continue;
		}
		if (add_to_epoll(epoll_fd, listen_sock, EPOLLIN) == -1)
			continue;
		servers[listen_sock] = config.servers[i];
	}

	if (servers.size() == 0)
	{
		Logger::error("could not register servers.");
		return;
	}

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
		Logger::debug("cgi handler finished exit_status= %d", h->getExitStatus());

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
	// TODO: handle timeout
	while (running)
	{
		int n_ev = epoll_wait(epoll_fd, events, MAX_EVENTS, 5000);
		// TODO: drop clients if timeout (now - last_activity > timout)
		if (n_ev <= 0) // possible error : EINTR
			continue;
		checkTimeouts();
		int ev;
		SOCKET fd;
		for (int i = 0; i < n_ev; i++)
		{
			ev = events[i].events;
			fd = static_cast<SOCKET>(events[i].data.fd);

			if (ev & (EPOLLERR | EPOLLHUP))
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
			{
				handleHttpResponse(fd);
			}
		}
		processFinishedCgi();
	}
}

// TODO handle internal server errors
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
	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &addr))
	{
		Logger::fatal("getaddrinfo failed");
	}

	int socket_listen =
		socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen))
		Logger::fatal("socket failed");
	// TODO: check why bind fails
	int opt = 1;
	if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &opt,
				   sizeof(opt)))
		Logger::error("setsockopt failed");

	if (bind(socket_listen, addr->ai_addr, addr->ai_addrlen))
		Logger::fatal("bind failed");
	freeaddrinfo(addr);

	Logger::info("Listening on http://%s:%s ...", host.c_str(), port.c_str());
	if (listen(socket_listen, SOMAXCONN))
		Logger::fatal("listen failed");

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
			if (errno == EAGAIN || errno == EWOULDBLOCK) // TODO check if this is allowed
				break;

			Logger::error("accept failed");
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
		if (add_to_epoll(epoll_fd, c->socket, EPOLLIN) == -1)
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
	if (bytes == 0)
	{
		removeClient(c);
		return;
	}
	else if (bytes < 0)
		return ;
	// Timeout updates
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
		modify_epoll(epoll_fd, c, EPOLLOUT);
	}
}

void Webserv::checkTimeouts()
{
	time_t now = time(NULL);

	std::map<SOCKET, Client *>::iterator it = clients.begin();
	while (it != clients.end())
	{
		SOCKET c = it->first;
		Client *cl = it->second;

		// skip timeout for clients with cgi still running (CgiManager handles it)
		if (cl->cgi_pending)
		{
			++it;
			continue;
		}

		bool drop = false;

		// Idle connection (e.g. client sent nothing for a long time)
		if (now - cl->last_activity > TIMEOUT)
		{
			Logger::debug("timeout for client(%d)", c);
			drop = true;
		}

		if (drop)
		{
			// removeClient erases from map; advance iterator carefully
			std::map<SOCKET, Client *>::iterator to_erase = it++;
			timeoutClient(to_erase->first);
		}
		else
		{
			++it;
		}
	}
}

void Webserv::timeoutClient(SOCKET c)
{
	HttpStatus status = clients[c]->machine.getRequest()->status;
	std::string resp =
		"HTTP/1.1 408 Request Timeout\r\n"
		"Content-Length: 0\r\n"
		"Connection: close\r\n"
		"\r\n";
		// "<html><body><h1>" + std::string(status.toString()) + "</h1></body></html>";
	send(c, resp.c_str(), resp.size(), 0);
	removeClient(c);
}

void Webserv::removeClient(SOCKET c)
{
	std::map<SOCKET, Client *>::iterator it = clients.find(c);
	if (it == clients.end())
	{
		return;
	}

	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c, NULL);
	Client *cl = it->second;
	clients.erase(c);
	close(c);
	delete cl;
	Logger::debug("dropping client(%d)", c);
}

void Webserv::handleHttpResponse(SOCKET c)
{
	if (!clients.count(c))
		return;

	Client *cl = clients[c];
	HttpRequest *req = cl->machine.getRequest();

	Logger::debug("handleHttpResponse: chunked=%d headers_sent=%d buffer_size=%zu",
				  cl->response.chunked, cl->response.headers_sent, cl->response.buffer.size());
	if (cl->cgi_pending == false &&
		(cl->machine.status.isMalformed()
		|| (cl->response.buffer.empty() && !cl->response.chunked && !cl->response.headers_sent)))
	{
		processRequest(cl);
	}

	// processRequest() may have dispatched a CGI script; wait for it to finish
	if (cl->cgi_pending)
		return;

	if (!cl->response.chunked)
	{
		if (cl->response.buffer.empty())
		{
			mimetype_map empty_types;
			mimetype_map &types = (cl->location && cl->location->shared_config)
								? cl->location->shared_config->types
								: empty_types;
			std::string content_type = getContentType(cl->file_path, types);
			Logger::debug("status code is : %d", req->status.asInt());
			cl->response.build(req->status, cl, content_type, cl->redirect_url);
		}
	}
	else
	{
		Logger::debug("Sending chunck...: offset=%zu", cl->response.offset);
		if (!cl->response.headers_sent)
		{
			send(c, cl->response.headers.c_str(),
				 cl->response.headers.size(), 0);
			cl->response.headers_sent = true;
			cl->last_activity = time(NULL);
			return;
		}
		// send next chunk of file
		char chunk[KIB(8)];
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

	ssize_t sent = send(
		c, cl->response.buffer.c_str() + cl->response.offset,
		cl->response.buffer.size() - cl->response.offset, 0);
	
	cl->last_activity = time(NULL);

	if (sent > 0)
		cl->response.offset += sent;

	if (cl->response.offset >= cl->response.buffer.size())
		removeClient(c);
}
