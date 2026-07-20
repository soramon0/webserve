#pragma once

#include "common.h"
#include "config/server.hpp"
#include "http/fsm.hpp"
#include <ctime>

class CgiManager;

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

	CgiManager*		cgiManager;
	bool			cgi_pending;
	//timeouts
	std::time_t		last_activity;
	std::time_t		request_start;
	Client();
	~Client();

};
