#ifndef CGIRESPONSE_HPP
#define CGIRESPONSE_HPP

#include <map>
#include <string>

struct CgiResponse
{
	int status_code;
	std::multimap<std::string, std::string> headers;
	std::string body;
};

CgiResponse parseCgiOutput(const std::string& cgi_output);

#endif