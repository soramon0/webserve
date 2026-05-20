#pragma once

#include "../../include/common.h"
#include "../config/server.hpp"
#include "request.hpp"

#define MAX_REQUEST_SIZE 2047

class Client
{
	socklen_t		addrlen;
	struct sockaddr addr;
	SOCKET			socket;

	Request			request;
	std::string		request_buffer;

	std::string		response_buffer;
	Server*			srv;

	//attributes for the complete request checker
	size_t			received;
	size_t			offset;
	size_t			header_size;
	bool			is_complete;
	int				is_header_parsed;

	// timouts
	time_t			last_activity;

	friend class Webserv;

public:
	Client();
	~Client();

	void	parseRequest();
	void	parseHeaders(std::string head);
	void	checkRequest();
	size_t	getMaxSize();

};
