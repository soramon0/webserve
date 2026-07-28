#ifndef CGIMANAGER_HPP
#define CGIMANAGER_HPP

#include "CgiHandler.hpp"
#include "CgiDispatchInfo.hpp"
#include <sys/epoll.h>
#include "../config/shared_config.hpp"
#include <vector>
#include <fcntl.h>
#include <map>

struct Client;

#define CGI_TIMEOUT_SECONDS  10

class CgiManager
{
private:
	const int epoll_fd;
	std::vector<CgiHandler*> handlers;
	std::vector<CgiHandler*> pending_reap;
	std::map<int, CgiHandler*> fd_to_handler;

public:
	CgiManager(int epoll_fd);
	~CgiManager();

	bool owns(int fd) const;
	bool registerHandler(const HttpRequest* request, Client* client, const CgiDispatchInfo& info);
	void removeHandler(CgiHandler* handler);
	void onReadable(struct epoll_event& ev);
	void reapPending();
	void checkTimeouts();
	std::vector<CgiHandler*> claimAllFinished();

private:
	void deregisterEpoll(CgiHandler* handler);
	void moveToPendingReap(CgiHandler* handler);
	void timeoutActiveHandlers(time_t now);
	void timeoutPendingReap(time_t now);

	CgiManager(const CgiManager& other);
	CgiManager& operator=(const CgiManager& other);
};

#endif