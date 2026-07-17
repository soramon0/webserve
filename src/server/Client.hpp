#pragma once

#include "common.h"
#include "config/server.hpp"
#include "http/fsm.hpp"
#include "Response.hpp"
#include <ctime>

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
	std::string		file_path;

	FSM				machine;

	//timeouts
	std::time_t		last_activity;
	std::time_t		request_start;

	Response		resp;

	Client();
	~Client();
};
