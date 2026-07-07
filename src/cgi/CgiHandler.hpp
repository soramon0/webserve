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
	int exit_status;

	const char *body; //until it's parsed
	size_t body_len;
	size_t body_written;

	std::string cgi_output;

	CgiState state;
	const HttpRequest* request;

	CgiHandler(const CgiHandler& other);
	CgiHandler& operator=(const CgiHandler& other);

public:
	CgiHandler(const HttpRequest* request,const char *body, size_t body_len);
	~CgiHandler();

	bool start(const std::string& interpreter_path, const std::string& script_path);
	void writeBody();
	void readOutput();
	bool waitChild();

	CgiState getCgiState() const;
	std::string getCgiOutput() const;
	int getWriteFd() const;
	int getReadFd() const;
	int getExitStatus() const;
};

#endif