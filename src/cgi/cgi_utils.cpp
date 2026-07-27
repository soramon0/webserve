#include "cgi_utils.hpp"
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>
#include "../server/Client.hpp"
#include "logger/log.hpp"
#include <cerrno>
#include <vector>

void close_wrapper(int &fd)
{
	if (fd == -1)
		return;
	close(fd);
	fd = -1;
}

static std::string normalisePath(const std::string &path)
{
	bool absolute = !path.empty() && path[0] == '/';
	std::vector<std::string> parts;
	size_t pos = 0;

	while (pos < path.length())
	{
		size_t next = path.find('/', pos);
		std::string segment = (next == std::string::npos) ? path.substr(pos) : path.substr(pos, next - pos);
		pos = (next == std::string::npos) ? path.length() : next + 1;

		if (segment.empty() || segment == ".")
			continue;
		if (segment == "..")
		{
			if (!parts.empty() && parts.back() != "..")
				parts.pop_back();
			else if (!absolute)
				parts.push_back(segment);
			continue;
		}
		parts.push_back(segment);
	}

	std::string result = absolute ? "/" : "";
	for (size_t i = 0; i < parts.size(); i++)
	{
		result += parts[i];
		if (i + 1 < parts.size())
			result += "/";
	}
	if (result.empty())
		result = absolute ? "/" : ".";
	return (result);
}

bool resolveScriptPath(const std::string &root, const std::string &uri_path,
					   std::string &script_path, std::string &path_info)
{
	path_info.clear();
	script_path.clear();

	size_t pos = (!uri_path.empty() && uri_path[0] == '/') ? 1 : 0;
	std::string candidate_path = root;
	struct stat st;

	while (pos <= uri_path.length())
	{
		size_t next_slash = uri_path.find('/', pos);
		std::string segment;

		if (next_slash == std::string::npos)
		{
			segment = uri_path.substr(pos);
			pos = uri_path.length() + 1;
		}
		else
		{
			segment = uri_path.substr(pos, next_slash - pos);
			pos = next_slash + 1;
		}
		if (segment.empty())
			continue;

		candidate_path += "/" + segment;

		if (stat(candidate_path.c_str(), &st) == -1)
		{
			Logger::debug("resolveScriptPath(): candidate_path = %s, errno_str = %s", candidate_path.c_str(), strerror(errno));
			return (false);
		}

		if (S_ISREG(st.st_mode))
		{
			script_path = candidate_path;
			path_info = (next_slash == std::string::npos) ? "" : uri_path.substr(next_slash);
			return (true);
		}
	}
	return (false);
}

bool lookupInterpreter(const std::map<std::string, std::string> &cgi_pass,
					   const std::string &script_path, std::string &interpreter_path)
{
	size_t dot = script_path.find_last_of('.');
	if (dot == std::string::npos || dot == script_path.length() - 1)
		return (false);
	std::string ext = script_path.substr(dot);
	std::map<std::string, std::string>::const_iterator it = cgi_pass.find(ext);
	if (it == cgi_pass.end())
		return (false);
	interpreter_path = it->second;
	return (true);
}

void resolveServerVars(const Client *cl, std::string &server_name, std::string &server_port)
{
	server_name = cl->srv->interface;

	std::ostringstream oss;
	oss << cl->srv->port;
	server_port = oss.str();
}

bool dispatchCgi(const std::string &root, const std::string &uri_path,
				 const std::map<std::string, std::string> &cgi_pass,
				 CgiDispatchInfo &info)
{
	info.resolved_root = normalisePath(root);
	if (!resolveScriptPath(info.resolved_root, uri_path, info.script_path, info.path_info))
		return (false);
	if (!lookupInterpreter(cgi_pass, info.script_path, info.interpreter_path))
		return (false);
	return (true);
}

CgiDispatchResult tryDispatchCgi(Client *cl, CgiManager &manager)
{
	HttpRequest *req = cl->machine.getRequest();
	std::string uri_path(req->uri.data(), req->uri.length());

	const std::string &loc_path = cl->location->path;
	std::string dispatch_uri = uri_path;

	// only strip the location prefix for directory locations
	if (uri_path.size() > loc_path.size() &&
		uri_path.compare(0, loc_path.size(), loc_path) == 0)
	{
		dispatch_uri = uri_path.substr(loc_path.size());
		if (dispatch_uri.empty() || dispatch_uri[0] != '/')
			dispatch_uri = "/" + dispatch_uri;
	}

	CgiDispatchInfo info;
	if (!dispatchCgi(cl->location->shared_config->root, dispatch_uri,
					 cl->location->shared_config->cgi_pass, info))
		return (NOT_CGI);

	resolveServerVars(cl, info.server_name, info.server_port);
	if (!manager.registerHandler(req, cl, info))
		return (CGI_DISPATCH_FAILED);

	cl->cgi_pending = true;
	return (CGI_DISPATCHED);
}

CgiDispatchResult tryDispatchResolvedCgi(Client *cl, CgiManager &manager, const std::string &resolved_file_path)
{
	HttpRequest *req = cl->machine.getRequest();
	CgiDispatchInfo info;

	info.resolved_root = normalisePath(cl->location->shared_config->root);
	info.script_path = normalisePath(resolved_file_path);

	if (!lookupInterpreter(cl->location->shared_config->cgi_pass, info.script_path, info.interpreter_path))
		return (NOT_CGI);

	info.path_info = "";
	resolveServerVars(cl, info.server_name, info.server_port);

	if (!manager.registerHandler(req, cl, info))
		return (CGI_DISPATCH_FAILED);

	cl->cgi_pending = true;
	return (CGI_DISPATCHED);
}