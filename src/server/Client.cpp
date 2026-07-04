#include "Client.hpp"
#include <string>
#include <sstream>
#include <cstdlib>

#define CRLF "\r\n"
#define BLANKLINE "\r\n\r\n"

Client::Client() : received(0), is_complete(0), offset(0),
					is_header_parsed(0), machine()
{
	addrlen = sizeof(addr);
}

Client::~Client() {
	this->machine.clear();
}

void	Client::parseHeaders(std::string head)
{
	std::string url;
	std::stringstream s(head);

	s >> request.method;
	s >> url;

	size_t p = url.find('?');
	if (p == std::string::npos)
		request.path = url;
	else
	{
		request.path = url.substr(0, p);
		request.query = url.substr(p + 1);
	}
	s >> request.protocol;
	std::string header;
	while (std::getline(s, header))
	{
		if (header.empty())
			break;
		header.erase(header.size() - 1);  // Remove \r
		size_t k_end = header.find(':');
		if (k_end == std::string::npos)
			continue ;
		request.headers[header.substr(0, k_end)] = header.substr(k_end + 2);
	}
	is_header_parsed = 1;
}

void	Client::checkRequest()
{
	if (request.headers.count("Content-Length"))
	{
		std::stringstream ss(request.headers["Content-Length"]);
		size_t value;
		ss >> value;
		if (value <= request.body.size())
			is_complete = true;
	}
	else if (request.method != "POST")
		is_complete = true;
}

void	Client::parseRequest()
{
	if (is_header_parsed)
	{
		request.body += request_buffer.substr(offset);
		checkRequest();
	}
	else
	{
		size_t hpos = request_buffer.find(BLANKLINE);

		if (hpos != std::string::npos)
		{
			parseHeaders(request_buffer.substr(0, hpos));
			header_size = hpos + 4;
			request.body = request_buffer.substr(hpos + 4);
			offset = request_buffer.size();
			checkRequest();
		}
	}
}

size_t Client::getMaxSize()
{
	size_t body_size = 0;

	if (!is_header_parsed)
		return MAX_REQUEST_SIZE;
	if (srv->locations.count(request.path))
		body_size = srv->locations[request.path].shared_config->client_max_body_size;
	return (header_size + body_size);
}
