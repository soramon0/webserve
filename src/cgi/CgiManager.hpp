#ifndef CGIMANAGER_HPP
#define CGIMANAGER_HPP

#include "../config/shared_config.hpp"
#include "Request.hpp"
#include "CgiHandler.hpp"
#include <map>

class CgiManager
{
private:
	std::map<int, CgiHandler*>		_cgiHandlers;
	std::map<int, int>				_cgiToClient;
	int&							_epoll_fd;

	
	std::map<std::string, std::string>	buildEnv(const Request& req, const SharedConfig& conf);
	std::string 						getExtension(const std::string& path);

public:
	CgiManager(int& epoll_fd);

	int									handleRequest(const Request& req, const SharedConfig& conf, int client_fd);
	std::string							pipeReady(int pipe_fd);
	bool								isCgiFd(int fd);
};

#endif