#include "CgiHandler.hpp"
#include "cgi_utils.hpp"
#include <sys/wait.h>
#include <sys/types.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <fcntl.h>
#include <sstream>
#include <vector>

CgiHandler::CgiHandler(const HttpRequest *request, const char *body, size_t body_len)
	: pid(-1), exit_status(0), body(body), body_len(body_len), body_written(0), cgi_output(""),
	  state(WRITING_BODY), request(request)
{
	pipe_in[0] = -1;
	pipe_in[1] = -1;
	pipe_out[0] = -1;
	pipe_out[1] = -1;
}

CgiHandler::~CgiHandler()
{
	close_wrapper(pipe_in[0]); close_wrapper(pipe_in[1]); close_wrapper(pipe_out[0]); close_wrapper(pipe_out[1]);

	if (pid > 0 && state != CGI_DONE)
	{
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
	}
}

char **CgiHandler::buildEnvp(const std::string &server_name, const std::string &server_port) const
{
	std::vector<std::string> vect_envp;

	vect_envp.push_back("REQUEST_METHOD=" + request->method.toString());
	vect_envp.push_back("SERVER_PROTOCOL=" + request->version.toString());
	vect_envp.push_back("GATEWAY_INTERFACE=CGI/1.1");
	vect_envp.push_back("SERVER_NAME=" + server_name);
	vect_envp.push_back("SERVER_PORT=" + server_port);

	if (body_len > 0)
	{
		std::ostringstream oss;
		oss << body_len;
		vect_envp.push_back("CONTENT_LENGTH=" + oss.str());
	}

	const StringView *ct = request->headers.get("content-type");
	if (ct != NULL)
		vect_envp.push_back("CONTENT_TYPE=" + std::string(ct->data(), ct->length()));

	std::string path, query_string;
	splitQueryString(request->uri, path, query_string);
	vect_envp.push_back("SCRIPT_NAME=" + path);
	vect_envp.push_back("QUERY_STRING=" + query_string);

	Headers::AllRange range = request->headers.all();
	std::string last_key;
	for (Headers::AllRange::first_type it = range.first; it != range.second; ++it)
	{
		std::string key(it->first.data(), it->first.length());
		std::string value(it->second.data(), it->second.length());

		if (key == "content-type" || key == "content-length" || key == "authorization")
			continue;

		if (key == last_key)
			vect_envp.back() += (key == "cookie" ? "; " : ",") + value;
		else
		{
			vect_envp.push_back(toHttpEnvName(key) + "=" + value);
			last_key = key;
		}
	}

	char **envp = vectorToEnvp(vect_envp);
	return (envp);
}

bool CgiHandler::start(const std::string &interpreter_path, const std::string &script_path,
					   const std::string &server_name, const std::string &server_port)
{
	if (pipe(pipe_in) == -1) { state = CGI_ERROR; return (false); }
	if (pipe(pipe_out) == -1) { close_wrapper(pipe_in[0]); close_wrapper(pipe_in[1]); state = CGI_ERROR; return (false); }

	if (fcntl(pipe_in[1], F_SETFD, FD_CLOEXEC) == -1) { close_wrapper(pipe_in[0]); close_wrapper(pipe_in[1]); close_wrapper(pipe_out[0]); close_wrapper(pipe_out[1]); state = CGI_ERROR; return (false); }
	if (fcntl(pipe_out[0], F_SETFD, FD_CLOEXEC) == -1) { close_wrapper(pipe_in[0]); close_wrapper(pipe_in[1]); close_wrapper(pipe_out[0]); close_wrapper(pipe_out[1]); state = CGI_ERROR; return (false);}

	if (interpreter_path.empty() || interpreter_path[0] != '/') { close_wrapper(pipe_in[0]); close_wrapper(pipe_in[1]); close_wrapper(pipe_out[0]); close_wrapper(pipe_out[1]); state = CGI_ERROR; return (false);}

	char *argv[3];
	argv[0] = const_cast<char *>(interpreter_path.c_str());
	argv[1] = const_cast<char *>(script_path.c_str());
	argv[2] = NULL;
	char **envp = buildEnvp(server_name, server_port);

	size_t pos = script_path.find_last_of('/');
	if (pos == std::string::npos)
	{
		freeEnvp(envp);
		close_wrapper(pipe_in[0]); close_wrapper(pipe_in[1]);
		close_wrapper(pipe_out[0]); close_wrapper(pipe_out[1]);
		state = CGI_ERROR;
		return (false);
	}
	std::string dir = script_path.substr(0, pos + 1);

	pid = fork();
	if (pid == -1) { freeEnvp(envp); close_wrapper(pipe_in[0]); close_wrapper(pipe_in[1]); close_wrapper(pipe_out[0]); close_wrapper(pipe_out[1]); state = CGI_ERROR; return (false); }
	else if (pid == 0)
	{
		close_wrapper(pipe_in[1]); close_wrapper(pipe_out[0]);

		if (dup2(pipe_in[0], STDIN_FILENO) == -1) _exit(1);
		if (dup2(pipe_out[1], STDOUT_FILENO) == -1) _exit(1);
		close_wrapper(pipe_in[0]); close_wrapper(pipe_out[1]);

		if (chdir(dir.c_str()) == -1) _exit(1);

		if (execve(argv[0], argv, envp) == -1) _exit(127);
	}
	start_time = time(NULL);
	close_wrapper(pipe_in[0]); close_wrapper(pipe_out[1]);
	freeEnvp(envp);
	return (true);
}

void CgiHandler::writeBody()
{
	ssize_t n = write(pipe_in[1], body + body_written, body_len - body_written);

	if (n > 0)
	{
		body_written += n;
		if (body_written == body_len)
		{
			close_wrapper(pipe_in[1]);
			state = READING_OUTPUT;
		}
		return ;
	}
	if (n == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;
		state = CGI_ERROR;
	}
}

void CgiHandler::readOutput()
{
	char buf[4096];
	ssize_t n = read(pipe_out[0], buf, sizeof(buf));
	if (n > 0)
		cgi_output.append(buf, n);
	else if (n == 0)
	{
		close_wrapper(pipe_out[0]);
		if (waitChild())
			state = CGI_DONE;
	}
	else if (n == -1)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return ;
		state = CGI_ERROR;
	}
}

bool CgiHandler::waitChild()
{
	int status;
	if (pid <= 0)
		return (false);
	pid_t ret = waitpid(pid, &status, WNOHANG);
	if (ret == 0)
		return (false);
	if (ret == pid)
	{
		if (WIFEXITED(status))
			exit_status = WEXITSTATUS(status);
		else if (WIFSIGNALED(status))
			exit_status = 128 + WTERMSIG(status);
		pid = -1;
		return (true);
	}
	if (ret == -1)
	{
		state = CGI_ERROR;
		return (false);
	}
	return (false);
}

bool CgiHandler::reap()
{
	if (waitChild())
	{
		state = CGI_DONE;
		return (true);
	}
	return (false);
}

void CgiHandler::timeoutKill()
{
	if (pid > 0)
	{
		kill(pid, SIGKILL);
		int status;
		waitpid(pid, &status, 0);
		pid = -1;
	}
	state = CGI_ERROR;
}

CgiState CgiHandler::getCgiState() const { return (state); }
const std::string& CgiHandler::getCgiOutput() const { return (cgi_output); }
int CgiHandler::getWriteFd() const { return (pipe_in[1]); }
int CgiHandler::getReadFd() const { return (pipe_out[0]); }
int CgiHandler::getExitStatus() const { return (exit_status); }
const HttpRequest* CgiHandler::getRequest() const { return (request); }
time_t CgiHandler::getStartTime() const { return (start_time); }