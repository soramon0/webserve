#include "CgiManager.hpp"
#include <sstream>

CgiManager::CgiManager(int& epoll_fd) : _epoll_fd(epoll_fd) {}

std::map<std::string, std::string> CgiManager::buildEnv(const Request& req, const SharedConfig& conf)
{
	std::map<std::string, std::string> env;

	env["REQUEST_METHOD"] = req.getMethod();
	env["CONTENT_TYPE"] = req.getContentType();
	env["QUERY_STRING"] = req.getQuery();
	env["SCRIPT_FILENAME"] = conf.root +  req.getPath();

	std::ostringstream ss;
	ss << req.getBody().size();
	env["CONTENT_LENGTH"] = ss.str();

	return (env);
}

std::string CgiManager::getExtension(const std::string& path)
{
	size_t pos = path.find_last_of('.');
	if (pos == std::string::npos)
		return ("");
	std::string extension = path.substr(pos);
	return (extension);
}

int CgiManager::handleRequest(const Request& req, const SharedConfig& conf, int client_fd)
{
	std::map<std::string, std::string> env = buildEnv(req, conf);
	std::string interpreter = conf.cgi_pass[getExtension(req.getPath())];

	CgiHandler cgi(req.getPath(), interpreter, req.getBody(), env);
	return (cgi.start());
}


bool	CgiManager::isCgiFd(int fd)
{
	return (_cgiHandlers.count(fd));
}