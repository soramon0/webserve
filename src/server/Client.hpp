#pragma once

#include "common.h"
#include "request.hpp"
#include "http/http_request.hpp"
#include "config/server.hpp"
#include "http/fsm.hpp"

#define MAX_REQUEST_SIZE 2047

class Client
{
	socklen_t		addrlen;
	struct sockaddr addr;
	SOCKET			socket;
	size_t			received;
	Request			request;
	Server*			srv;
	std::string		request_buffer;

	//attributes for the complete requecst checker
	bool			is_complete;
	size_t			offset;
	int				is_header_parsed;
	size_t			header_size;

	FSM machine;

	friend class Webserv;

public:
	Client();
	~Client();

	void	parseRequest();
	void	parseHeaders(std::string head);
	void	checkRequest();
	size_t	getMaxSize();

};
