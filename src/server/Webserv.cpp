#include "Webserv.hpp"
#include "utils.hpp"
#include "../logger/log.hpp"
#include <sys/epoll.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>

#define MAX_EVENTS 64
#define BLANKLINE "\r\n\r\n"
#define HELLO_WORLD_RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!"

Webserv::Webserv(Config _conf) : config(_conf) {}

Webserv::~Webserv() {}

void	Webserv::start()
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
		// TODO: close all opened fds before exiting
		if (add_to_epoll(epoll_fd, listen_sock, EPOLLIN) == -1)
			continue ;
		servers[listen_sock] = &config.servers[i];
	}

	eventLoop();
}

void	Webserv::eventLoop()
{
	struct epoll_event events[MAX_EVENTS];

	while (true)
	{
		int n_ev = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		std::cout << "hello\n";
		int ev, fd;
		for (int i = 0; i < n_ev; i++)
		{
			ev = events[i].events;
			fd = events[i].data.fd;

			if (ev & EPOLLIN)
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
			else if (ev & (EPOLLERR | EPOLLHUP))
			{
				// handle errors
			}
		}
	}
}

SOCKET	Webserv::createSocket(int id)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));

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
		Logger::fatal("Invalid socket error"); //TODO close everything

	int opt = 1;
	if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
		Logger::error("setsockopt failed");

	if (bind(socket_listen, addr->ai_addr, addr->ai_addrlen))
		Logger::fatal("Can't bind"); //TODO close everything
	freeaddrinfo(addr);

	std::cout << "Listening...\n";
	if (listen(socket_listen, SOMAXCONN))
		Logger::fatal("listen failed"); //TODO close everything

	return socket_listen;
}

void	Webserv::handleNewConnection(SOCKET srv)
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

		clients[c.socket] = c;
		Logger::info("client Connected...\n\n");
	}
}
void	Webserv::handleClientData(SOCKET c)
{
	Client& cl = clients[c];

	if (MAX_REQUEST_SIZE == cl.received)
	{
		//send error page 40x
	}

	int bytes = recv(cl.socket, cl.request + cl.received , MAX_REQUEST_SIZE - cl.received, 0);

	if (bytes <= 0)
	{
		clients.erase(c);
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c, NULL);
		close(c);
		return ;
	}

	cl.received += bytes;
	cl.request[cl.received] = 0;
	handleHttpRequest(c);
}


void	Webserv::handleHttpRequest(SOCKET c)
{
	Client& cl = clients[c];

	Logger::info("=============REQUEST===========\n%s", cl.request);
	if (strstr(cl.request, BLANKLINE))
	{
		modify_epoll(epoll_fd, c, EPOLLOUT);
		handleHttpResponse(c);
	}
}

void	Webserv::handleHttpResponse(SOCKET c)
{
	const char response[100] = HELLO_WORLD_RESPONSE;

	
	int n = send(c, response, strlen(response), 0);
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c, NULL);
	close(c);
	Logger::info("Client disconneted...& send n = %d bytes", n);
}
