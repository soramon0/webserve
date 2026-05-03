#pragma once

#include "../../include/common.h"
#include "../config/config.hpp"
#include "Client.hpp"
#include <vector>
#include <map>

class Webserv
{
private:
	Config						config;
	int							epoll_fd;
	std::map<SOCKET, Client>	clients;
	std::map<SOCKET, Server*>	servers;
public:
	Webserv(Config _conf);
	~Webserv();

	void	start();
	SOCKET	createSocket(int id);
	void	handleNewConnection(SOCKET srv);
	void	handleHttpRequest(SOCKET c);
	void	handleHttpResponse(SOCKET c);
	void	handleClientData(SOCKET c);
	void	eventLoop();
};

