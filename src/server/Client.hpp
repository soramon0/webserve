#pragma once

#include "common.h"
#include "config/server.hpp"
#include "http/fsm.hpp"
#include "Response.hpp"
#include <ctime>
#include "cgi/CgiResponse.hpp"

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
	std::string		file_path;

	FSM				machine;

	//cgi fields
	CgiManager*		cgiManager;
	bool			cgi_pending;

	//timeouts
	std::time_t		last_activity;

	Response		response;

	Client();
	~Client();

};
