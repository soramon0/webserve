#pragma once

#include "../../include/common.h"

#define MAX_REQUEST_SIZE 2047

struct Client
{
	socklen_t		addrlen;
	struct sockaddr addr;
	SOCKET			socket;
	char			request[MAX_REQUEST_SIZE + 1];
	int				received;

	Client();
	~Client();
};
