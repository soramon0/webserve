#pragma once

#include "common.h"
#include "config/server.hpp"
#include "http/fsm.hpp"

struct Client
{
	socklen_t		addrlen;
	struct sockaddr addr;
	SOCKET			socket;
	Server*			srv;

	//new fields
	Location*		location;
	std::string		redirect_url;
	std::string		response_body;

	FSM				machine;

	Client();
	~Client();
};
