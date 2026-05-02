#include "Webserv.hpp"
#include "utils.hpp"
#include <sys/epoll.h>
#include <iostream>
#include <sstream>
#include <string>

#define MAX_EVENTS 64
#define BLANKLINE "\r\n\r\n"

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

	int socket_listen = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen))
		Logger::fatal("Invalid socket error"); //TODO close everything

	int opt = 1;
	if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
		Logger::error("setsockopt failed");

	if (bind(socket_listen, address->ai_addr, address->ai_addrlen))
		Logger::fatal("Can't bind"); //TODO close everything
	freeaddrinfo(address);

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
		Logger::info("client Connected...\n\n")

	}
}
void	Webserv::handleHttpRequest(SOCKET c);
void	Webserv::handleHttpResponse(SOCKET c);
void	Webserv::handleClientData(SOCKET c);
