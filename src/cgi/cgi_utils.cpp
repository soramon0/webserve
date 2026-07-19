#include "cgi_utils.hpp"
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <sstream>
#include "../server/Client.hpp"

static size_t findChar(const StringView &sv, char c)
{
	for (size_t pos = 0; pos < sv.length(); pos++)
	{
		if (c == sv.data()[pos])
			return (pos);
	}
	return (sv.length());
}

void splitQueryString(const StringView &uri, std::string &path, std::string &query_string)
{
	size_t pos = findChar(uri, '?');
	path = std::string(uri.data(), pos);
	if (pos == uri.length())
	{
		query_string = "";
		return;
	}
	else
	{
		query_string = std::string(uri.data() + pos + 1, uri.length() - pos - 1);
		return;
	}
}

void close_wrapper(int &fd)
{
	if (fd == -1)
		return;
	close(fd);
	fd = -1;
}

bool resolveScriptPath(const std::string &root, const std::string &uri_path,
					   std::string &script_path, std::string &path_info)
{
	path_info.clear();
	script_path.clear();
	size_t pos = 0;
	if (uri_path[0] == '/')
		pos++;
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
			return (false);
		if (S_ISREG(st.st_mode))
		{
			script_path = candidate_path;
			if (next_slash == std::string::npos)
				path_info = "";
			else
				path_info = uri_path.substr(next_slash);
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
	server_name = cl->getServer()->interface;

	std::ostringstream oss;
	oss << cl->getServer()->port;
	server_port = oss.str();
}

bool dispatchCgi(const std::string &root, const std::string &uri_path,
				 const std::map<std::string, std::string> &cgi_pass,
				 CgiDispatchInfo &info)
{
	if (!resolveScriptPath(root, uri_path, info.script_path, info.path_info))
		return (false);
	if (!lookupInterpreter(cgi_pass, info.script_path, info.interpreter_path))
		return (false);
	return (true);
}
