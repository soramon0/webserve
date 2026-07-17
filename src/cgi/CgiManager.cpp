#include "CgiManager.hpp"
#include "cgi_utils.hpp"
#include <algorithm>
#include <sstream>

CgiManager::CgiManager(int epoll_fd) : epoll_fd(epoll_fd) {}

CgiManager::~CgiManager()
{
	for (std::vector<CgiHandler*>::size_type i = 0; i < pending_reap.size(); i++)
		delete (pending_reap[i]);
	for (std::vector<CgiHandler*>::size_type i = 0; i < handlers.size(); i++)
		delete (handlers[i]);
}

bool CgiManager::owns(int fd) const
{
	for (std::vector<CgiHandler *>::size_type i = 0; i < handlers.size(); i++)
	{
		if (fd == handlers[i]->getReadFd())
			return (true);
	}
	return (false);
}

bool CgiManager::registerHandler(const HttpRequest *request,
								 const std::string &interpreter_path, const std::string &script_path,
								 const std::string &server_name, const std::string &server_port)
{
	CgiHandler* handler = new CgiHandler(request);
	if (handler->start(interpreter_path, script_path, server_name, server_port))
	{
		if (fcntl(handler->getReadFd(), F_SETFL, O_NONBLOCK) == -1) { delete handler; return (false);}

		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.ptr = handler;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handler->getReadFd(), &ev) == -1) { delete handler; return (false);}
		handlers.push_back(handler);
		return (true);
	}
	delete handler;
	return (false);
}

void CgiManager::deregisterEpoll(CgiHandler* handler)
{
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

void CgiManager::dispatch(struct epoll_event& ev)
{
	CgiHandler* handler = static_cast<CgiHandler*>(ev.data.ptr);

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

void CgiManager::reapPending()
{
	std::vector<CgiHandler*>::iterator it = pending_reap.begin();
	while (it != pending_reap.end())
	{
		if ((*it)->reap())
		{
			handlers.push_back(*it);
			it = pending_reap.erase(it);
		}
		else
			++it;
	}
}

CgiHandler* CgiManager::claim(const HttpRequest* request)
{
	for (std::vector<CgiHandler *>::iterator it = handlers.begin(); it != handlers.end(); it++)
	{
		if ((*it)->getRequest() == request && ((*it)->getCgiState() == CGI_DONE || (*it)->getCgiState() == CGI_ERROR))
		{
			CgiHandler* result = *it;
			handlers.erase(it);
			return (result);
		}
	}
	return (NULL);
}

void CgiManager::checkTimeouts()
{
	time_t now = time(NULL);
	for (std::vector<CgiHandler*>::size_type i = 0; i < handlers.size(); i++)
	{
		CgiState s = handlers[i]->getCgiState();
		if (s == READING_OUTPUT && now - handlers[i]->getStartTime() > CGI_TIMEOUT_SECONDS)
		{
			deregisterEpoll(handlers[i]);
			handlers[i]->timeoutKill();
		}
	}
	std::vector<CgiHandler*>::iterator it = pending_reap.begin();
	while(it != pending_reap.end())
	{
		if (now - (*it)->getStartTime() > CGI_TIMEOUT_SECONDS)
		{
			(*it)->timeoutKill();
			handlers.push_back(*it);
			it = pending_reap.erase(it);
		}
		else
			++it;
	}
}
