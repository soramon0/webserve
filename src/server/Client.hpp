#pragma once

#include "common.h"
#include "config/server.hpp"
#include "http/fsm.hpp"

class Client
{
	socklen_t		addrlen;
	struct sockaddr addr;
	SOCKET			socket;
	Server*			srv;

	FSM machine;

	friend class Webserv;

public:
	Client();
	~Client();

};
