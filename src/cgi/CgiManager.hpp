#ifndef CGIMANAGER_HPP
#define CGIMANAGER_HPP

#include "CgiHandler.hpp"
#include <sys/epoll.h>
#include "../config/shared_config.hpp"
#include <vector>
#include <fcntl.h>

#define CGI_TIMEOUT_SECONDS  10

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
	bool registerHandler(const HttpRequest* request,
						const std::string& interpreter_path, const std::string& script_path,
						const std::string& server_name, const std::string& server_port);
	void removeHandler(CgiHandler* handler);
	void dispatch(struct epoll_event& ev);
	CgiHandler* claim(const HttpRequest* request);
	void reapPending();
	void checkTimeouts();

private:
	void deregisterEpoll(CgiHandler* handler);

	CgiManager(const CgiManager& other);
	CgiManager& operator=(const CgiManager& other);
};

#endif