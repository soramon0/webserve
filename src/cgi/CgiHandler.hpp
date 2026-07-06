#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include "../http/http_request.hpp"

enum CgiState
{
	WRITING_BODY, READING_OUTPUT, CGI_DONE, CGI_ERROR
};

class CgiHandler
{
private:
	int pipe_in[2];
	int pipe_out[2];
	pid_t pid;

	const char *body; //until it's parsed
	size_t body_len;
	size_t body_written;

	std::string cgi_output;

	CgiState state;
	const HttpRequest* request;

	CgiHandler(const CgiHandler& other);
	CgiHandler& operator=(const CgiHandler& other);

public:
	CgiHandler();
	CgiHandler(const HttpRequest* request,const char *body, size_t body_len);
	~CgiHandler();
};

#endif