#pragma once

#include "../../include/common.h"
#include "request.hpp"
#include "../config/server.hpp"

#define MAX_REQUEST_SIZE 2047

class Client
{
	socklen_t		addrlen;
	struct sockaddr addr;
	SOCKET			socket;
	// char			tmp_request[MAX_REQUEST_SIZE + 1];
	size_t			received;
	Request			request;

	std::string		request_buffer;
	Server*			srv;
	bool			is_complete;
	size_t			offset;
	int				is_header_parsed;

	friend class Webserv;
public:
	Client();
	~Client();

	void	parseRequest();
	void	parseHeaders(std::string head);
	void	checkRequest();
};
