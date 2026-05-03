#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>

struct Request
{
	std::string method;
	std::string path;
	std::string	queryString;
	std::string body;
	std::string contentType;
	std::string protocol;
};

#endif