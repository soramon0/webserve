#pragma once

#include "../../include/common.h"
#include <vector>
#include <map>

class Webserv
{
private:
	Config						conf;
	int							epoll_fd;
	// std::vector<int>			listning_socks;
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

