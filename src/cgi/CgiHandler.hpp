#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <ctime>
#include <string>
#include <map>
#include "../http/http_request.hpp"

enum CgiState
{
	READING_OUTPUT, CGI_DONE, CGI_ERROR
};

class CgiHandler
{
private:
	int pipe_out[2];
	pid_t pid;
	int exit_status;
	std::string cgi_output;
	CgiState state;
	const HttpRequest* request;
	time_t start_time;

public:
	CgiHandler(const HttpRequest* request);
	~CgiHandler();

	bool start(const std::string& interpreter_path, const std::string& script_path,
			const std::string& server_name, const std::string& server_port);
	void readOutput();
	bool waitChild();
	bool reap();
	void timeoutKill();


	//getters
	CgiState getCgiState() const;
	const std::string& getCgiOutput() const;
	int getReadFd() const;
	int getExitStatus() const;
	const HttpRequest* getRequest() const;
	time_t getStartTime() const;

private:
	char** buildEnvp(const std::string& server_name, const std::string& server_port) const;

	CgiHandler(const CgiHandler& other);
	CgiHandler& operator=(const CgiHandler& other);
};

#endif