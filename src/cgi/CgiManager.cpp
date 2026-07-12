#include "CgiManager.hpp"
#include "cgi_utils.hpp"
#include <algorithm>
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

void CgiManager::deregisterEpoll(CgiHandler* handler)
{
	if (handler->getWriteFd() != -1)
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, handler->getWriteFd(), NULL);
	if (handler->getReadFd() != -1)
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, handler->getReadFd(), NULL);
}

void CgiManager::removeHandler(CgiHandler* handler)
{
	deregisterEpoll(handler);
	std::vector<CgiHandler*>::iterator it = std::find(handlers.begin(), handlers.end(), handler);
	if (it != handlers.end())
		handlers.erase(it);
	delete handler;
}

void CgiManager::handleWriteResult(CgiHandler* handler)
{
	if (handler->getCgiState() == READING_OUTPUT)
	{
		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.ptr = handler;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handler->getReadFd(), &ev) == -1)
			removeHandler(handler);
		return ;
	}
	else if (handler->getCgiState() == CGI_ERROR)
		deregisterEpoll(handler);
	return ;
}

void CgiManager::dispatch(struct epoll_event& ev)
{
	CgiHandler* handler = static_cast<CgiHandler*>(ev.data.ptr);
	if (handler->getCgiState() == WRITING_BODY)
	{
		handler->writeBody();
		handleWriteResult(handler);
		return ;
	}
	if (handler->getCgiState() == READING_OUTPUT)
	{
		handler->readOutput();
		if (handler->getCgiState() == CGI_ERROR)
		{
			deregisterEpoll(handler);
			return ;
		}
		if (handler->getCgiState() == READING_OUTPUT && handler->getReadFd() == -1)
		{
			std::vector<CgiHandler*>::iterator it = std::find(handlers.begin(), handlers.end(), handler);
			if (it != handlers.end())
				handlers.erase(it);
			pending_reap.push_back(handler);
		}
	}
	return ;
}

CgiHandler* CgiManager::claimFrom(std::vector<CgiHandler*>& container, const HttpRequest* request)
{
	for (std::vector<CgiHandler *>::iterator it = container.begin(); it != container.end(); it++)
	{
		if ((*it)->getRequest() == request && ((*it)->getCgiState() == CGI_DONE || (*it)->getCgiState() == CGI_ERROR))
		{
			CgiHandler* result = *it;
			container.erase(it);
			return (result);
		}
	}
	return (NULL);
}

CgiHandler* CgiManager::claim(const HttpRequest* request) 
{
	CgiHandler* ret = claimFrom(handlers, request);
	if (ret == NULL)
		ret = claimFrom(pending_reap, request);
	return (ret);
}