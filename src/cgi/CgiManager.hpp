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
	std::vector<CgiHandler*> pending_reap;

public:
	CgiManager(int epoll_fd);
	~CgiManager();

	bool owns(int fd) const;
	bool registerHandler(const HttpRequest* request, const char* body, size_t body_len,
						const std::string& interpreter_path, const std::string& script_path,
						const std::string& server_name, const std::string& server_port);
	void removeHandler(CgiHandler* handler);
	void dispatch(struct epoll_event& ev);
	CgiHandler* claim(const HttpRequest* request);
	void reapPending();

private:
	void deregisterEpoll(CgiHandler* handler);
	void handleWriteResult(CgiHandler* handler);
};

#endif