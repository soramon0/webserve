#ifndef CGIMANAGER_HPP
#define CGIMANAGER_HPP

#include "CgiHandler.hpp"
#include <sys/epoll.h>
#include "../config/shared_config.hpp"
#include <vector>

class CgiManager
{
private:
	const int epoll_fd;
	std::vector<CgiHandler*> handlers;

public:
	CgiManager(int epoll_fd);
	~CgiManager();

	bool owns(int fd) const;

private:

};

#endif