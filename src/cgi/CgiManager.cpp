#include "CgiManager.hpp"
#include <sstream>

CgiManager::CgiManager(int epoll_fd) : epoll_fd(epoll_fd) {}

CgiManager::owns(int fd) const
{
	for (std::vector<CgiHandler*>::size_type i = 0; i < handlers.size(); i++)
	{
		if (fd == handlers[i]->getReadFd() || fd == handlers[i]->getWriteFd())
			return (true);
	}
	return (false);
}