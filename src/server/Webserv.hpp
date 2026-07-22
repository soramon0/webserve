#pragma once

#include "../../include/common.h"
#include "../config/config.hpp"
#include "Client.hpp"
#include <map>

#define MAX_EVENTS 64
#define TIMEOUT 10

class CgiManager;

class Webserv
{
private:
	Config&						config;
	int							epoll_fd;
	std::map<SOCKET, Client*>	clients;
	std::map<SOCKET, Server*>	servers;
	CgiManager*					cgiManager;
public:
	Webserv(Config& _conf);
	~Webserv();

	void	start();
	SOCKET	createSocket(int id);
	void	handleNewConnection(SOCKET srv);
	void	handleHttpResponse(SOCKET c);
	void	handleClientData(SOCKET c);
	void	removeClient(SOCKET c);
	void	eventLoop();
	void	checkTimeouts();
	void	timeoutClient(SOCKET c);

private:
	//to process finished cgi requests 
	void processFinishedCgi();
};

