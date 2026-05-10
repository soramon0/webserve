#pragma once

#include "../../include/common.h"
#include "../config/config.hpp"
#include "Client.hpp"
#include <vector>
#include <map>


#define MAX_EVENTS 64

#define HELLO_WORLD_RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!"

class Webserv
{
private:
	Config*						config;
	int							epoll_fd;
	std::map<SOCKET, Client>	clients;
	std::map<SOCKET, Server*>	servers;
public:
	Webserv(Config* _conf);
	~Webserv();

	void	start();
	SOCKET	createSocket(int id);
	void	handleNewConnection(SOCKET srv);
	void	handleHttpRequest(SOCKET c);
	void	handleHttpResponse(SOCKET c);
	void	handleClientData(SOCKET c);
	void	removeClient(SOCKET c);
	void	eventLoop();
};

