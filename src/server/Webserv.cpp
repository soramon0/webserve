#include "Webserv.hpp"
#include "utils.hpp"
#include "../logger/log.hpp"
#include <sys/epoll.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <signal.h>

static int running = true;

Webserv::Webserv(Config _conf) : config(_conf) {}

Webserv::~Webserv()
{
	close(epoll_fd);
	// close all sockets before
	std::map<SOCKET, Client>::iterator it_cl = clients.begin();
	std::map<SOCKET, Server*>::iterator it_srv = servers.begin();
	while (it_cl != clients.end())
	{
		close(it_cl->first);
		++it_cl;
	}
	clients.clear();
	while (it_srv != servers.end())
	{
		close(it_srv->first);
		++it_srv;
	}
	servers.clear();
}

void Webserv::start()
{
	int srvlen = config.servers.size();

	epoll_fd = epoll_instance();
	for (int i = 0; i < srvlen; i++)
	{
		SOCKET listen_sock = createSocket(i);
		if (set_nonblocking(listen_sock) == -1)
		{
			Logger::error("Can't set non-blocking");
			continue ;
		}
		if (add_to_epoll(epoll_fd, listen_sock, EPOLLIN) == -1)
			continue ;
		servers[listen_sock] = &config.servers[i];
	}
	eventLoop();
}

void sigHandler(int sig)
{
	(void)sig;
	running = false;
}

void Webserv::eventLoop()
{
	struct epoll_event events[MAX_EVENTS];

	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);

	while (running)
	{
		int n_ev = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		int ev, fd;
		for (int i = 0; i < n_ev; i++)
		{
			ev = events[i].events;
			fd = events[i].data.fd;

			if (ev & (EPOLLERR | EPOLLHUP))
			{
				removeClient(fd);
				continue;
			}
			else if (ev & EPOLLIN)
			{
				if (servers.count(fd))
					handleNewConnection(fd);
				else
					handleClientData(fd);
			}
			else if (ev & EPOLLOUT)
			{
				handleHttpResponse(fd);
			}
		}
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
	os << config.servers[id].port;
	std::string port = os.str();
	std::string host = config.servers[id].interface;

	struct addrinfo *addr;
	getaddrinfo(host.c_str(), port.c_str(), &hints, &addr);

	int socket_listen = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen))
		Logger::fatal("socket failed");

	int opt = 1;
	if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
		Logger::error("setsockopt failed");

	if (bind(socket_listen, addr->ai_addr, addr->ai_addrlen))
		Logger::fatal("bind failed");
	freeaddrinfo(addr);

	Logger::info("Listening...");
	if (listen(socket_listen, SOMAXCONN))
		Logger::fatal("listen failed");

	return socket_listen;
}

void Webserv::handleNewConnection(SOCKET srv)
{
	while (true)
	{
		Client c;
		c.socket = accept(srv, &c.addr, &c.addrlen);
		c.received = 0;

		if (c.socket == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break ;
			Logger::error("accept failed");
			break ;
		}
		if (set_nonblocking(c.socket) == -1)
		{
			Logger::error("fcntl failed");
			close(c.socket);
			continue ;
		}
		if (add_to_epoll(epoll_fd, c.socket, EPOLLIN) == -1)
			continue ;
		c.srv = servers[srv];
		clients[c.socket] = c;
		Logger::info("client Connected...");
	}
}

void Webserv::handleClientData(SOCKET c)
{
	Client&	cl = clients[c];

	if (cl.received >= cl.getMaxSize())
	{
		//send error page 413, remove client
		removeClient(c);
		return;
	}

	char buffer[MAX_REQUEST_SIZE];
	int bytes = recv(cl.socket, buffer, sizeof(buffer), 0);

	if (bytes <= 0)
	{
		removeClient(c);
		return ;
	}
	cl.request_buffer.append(buffer, bytes);
	cl.received += bytes;
	handleHttpRequest(c);
}

void Webserv::removeClient(SOCKET c)
{
	if (!clients.count(c))
		return;
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c, NULL);
	close(c);
	clients.erase(c);
}

void Webserv::handleHttpRequest(SOCKET c)
{
	Client& cl = clients[c];

	cl.parseRequest();
	if (cl.is_complete)
	{
		cl.request.printRequest();
		modify_epoll(epoll_fd, c, EPOLLOUT);
	}
}

void Webserv::handleHttpResponse(SOCKET c)
{
	std::string response = HELLO_WORLD_RESPONSE;

	int n = send(c, response.c_str(), response.size(), 0);
	(void)n;
	removeClient(c);
	Logger::info("Client disconneted...\n");
}
