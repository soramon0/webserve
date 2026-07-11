#include "CgiManager.hpp"
#include <sstream>

CgiManager::CgiManager(int epoll_fd) : epoll_fd(epoll_fd) {}

bool CgiManager::owns(int fd) const
{
	for (std::vector<CgiHandler *>::size_type i = 0; i < handlers.size(); i++)
	{
		if (fd == handlers[i]->getReadFd() || fd == handlers[i]->getWriteFd())
			return (true);
	}
	return (false);
}

bool CgiManager::registerHandler(const HttpRequest *request, const char *body, size_t body_len,
								 const std::string &interpreter_path, const std::string &script_path,
								 const std::string &server_name, const std::string &server_port)
{
	CgiHandler* handler = new CgiHandler(request, body, body_len);
	if (handler->start(interpreter_path, script_path, server_name, server_port))
	{
		if (fcntl(handler->getWriteFd(), F_SETFL, O_NONBLOCK) == -1) { delete handler; return (false);}
		if (fcntl(handler->getReadFd(), F_SETFL, O_NONBLOCK) == -1) { delete handler; return (false);}

		struct epoll_event ev;
		ev.events = EPOLLOUT;
		ev.data.ptr = handler;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handler->getWriteFd(), &ev) == -1) { delete handler; return (false);}
		handlers.push_back(handler);
		return (true);
	}
	delete handler;
	return (false);
}