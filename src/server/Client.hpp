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
	size_t			received;
	Request			request;
	Server*			srv;
	std::string		request_buffer;
	std::string		response_buffer;

	//attributes for the complete request checker
	bool			is_complete;
	size_t			offset;
	int				is_header_parsed;
	size_t			header_size;

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
