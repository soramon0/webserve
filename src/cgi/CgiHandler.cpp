#include "CgiHandler.hpp"
#include "cgi_utils.hpp"
#include "Fd.hpp"
#include <sys/wait.h>
#include <sys/types.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <fcntl.h>
#include <sstream>
#include <vector>

CgiHandler::CgiHandler(const HttpRequest *request)
	: pid(-1), exit_status(0), cgi_output(""), state(READING_OUTPUT), request(request)
{
	pipe_out[0] = -1;
	pipe_out[1] = -1;
}

CgiHandler::~CgiHandler()
{
	close_wrapper(pipe_out[0]);
	close_wrapper(pipe_out[1]);

	if (pid > 0 && state != CGI_DONE)
	{
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
	}
}

std::string CgiHandler::toHttpEnvName(const std::string &key)
{
	std::string env_name = key;

	for (size_t i = 0; i < key.length(); i++)
	{
		if (key[i] == '-')
			env_name[i] = '_';
		else
			env_name[i] = std::toupper(static_cast<unsigned char>(key[i]));
	}
	env_name = "HTTP_" + env_name;
	return (env_name);
}

char **CgiHandler::vectorToEnvp(const std::vector<std::string> &vect)
{
	char **envp = new char *[vect.size() + 1];
	size_t i = 0;
	for (i = 0; i < vect.size(); i++)
	{
		envp[i] = new char[vect[i].size() + 1];
		std::strcpy(envp[i], vect[i].c_str());
	}
	envp[i] = NULL;
	return (envp);
}

void CgiHandler::addStandardVars(std::vector<std::string> &vect_envp,
								 const std::string &server_name, const std::string &server_port) const
{
	vect_envp.push_back("REQUEST_METHOD=" + request->method.toString());
	vect_envp.push_back("SERVER_PROTOCOL=" + request->version.toString());
	vect_envp.push_back("GATEWAY_INTERFACE=CGI/1.1");
	vect_envp.push_back("SERVER_NAME=" + server_name);
	vect_envp.push_back("SERVER_PORT=" + server_port);
}

void CgiHandler::addBodyVars(std::vector<std::string> &vect_envp) const
{
	if (request->body.size() > 0)
	{
		std::ostringstream oss;
		oss << request->body.size();
		vect_envp.push_back("CONTENT_LENGTH=" + oss.str());
	}
	const StringView *ct = request->headers.get("content-type");
	if (ct != NULL)
		vect_envp.push_back("CONTENT_TYPE=" + std::string(ct->data(), ct->length()));
}

void CgiHandler::addUriVars(std::vector<std::string> &vect_envp, const std::string& path_info) const
{
	std::string path, query_string;
	splitQueryString(request->uri, path, query_string);
	vect_envp.push_back("SCRIPT_NAME=" + path);
	vect_envp.push_back("QUERY_STRING=" + query_string);
	if (!path_info.empty())
		vect_envp.push_back("PATH_INFO=" + path_info);
}

void CgiHandler::addHeaderVars(std::vector<std::string> &vect_envp) const
{
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
}

void CgiHandler::freeEnvp(char **envp)
{
	for (size_t i = 0; envp[i] != NULL; i++)
		delete[] envp[i];
	delete[] envp;
}

char **CgiHandler::buildEnvp(const std::string &server_name, const std::string &server_port, const std::string& path_info) const
{
	std::vector<std::string> vect_envp;

	addStandardVars(vect_envp, server_name, server_port);
	addBodyVars(vect_envp);
	addUriVars(vect_envp, path_info);
	addHeaderVars(vect_envp);

	return (vectorToEnvp(vect_envp));
}

bool CgiHandler::start(const std::string &interpreter_path, const std::string &script_path,
					   const std::string &server_name, const std::string &server_port,
						const std::string& path_info)
{
	if (pipe(pipe_out) == -1)
	{
		state = CGI_ERROR;
		return (false);
	}
	Fd read_guard(pipe_out[0]);
	Fd write_guard(pipe_out[1]);

	if (fcntl(pipe_out[0], F_SETFD, FD_CLOEXEC) == -1)
	{
		state = CGI_ERROR;
		return (false);
	}

	int body_fd = open(request->body.getFilePath().c_str(), O_RDONLY);
	if (body_fd == -1)
	{
		state = CGI_ERROR;
		return (false);
	}
	Fd body_guard(body_fd);

	if (interpreter_path.empty() || interpreter_path[0] != '/')
	{
		state = CGI_ERROR;
		return (false);
	}

	char *argv[3];
	argv[0] = const_cast<char *>(interpreter_path.c_str());
	argv[1] = const_cast<char *>(script_path.c_str());
	argv[2] = NULL;
	char **envp = buildEnvp(server_name, server_port, path_info);

	size_t pos = script_path.find_last_of('/');
	if (pos == std::string::npos)
	{
		freeEnvp(envp);
		state = CGI_ERROR;
		return (false);
	}
	std::string dir = script_path.substr(0, pos + 1);

	pid = fork();
	if (pid == -1)
	{
		freeEnvp(envp);
		state = CGI_ERROR;
		return (false);
	}
	else if (pid == 0)
	{
		close_wrapper(pipe_out[0]);
		if (dup2(pipe_out[1], STDOUT_FILENO) == -1)
			_exit(1);
		if (dup2(body_fd, STDIN_FILENO) == -1)
			_exit(1);
		close_wrapper(pipe_out[1]);
		close_wrapper(body_fd);

		if (chdir(dir.c_str()) == -1)
			_exit(1);

		if (execve(argv[0], argv, envp) == -1)
			_exit(127);
	}
	start_time = time(NULL);
	freeEnvp(envp);
	read_guard.release();
	return (true);
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
		state = CGI_ERROR;
		return;
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

const std::string &CgiHandler::getCgiOutput() const { return (cgi_output); }

int CgiHandler::getReadFd() const { return (pipe_out[0]); }

int CgiHandler::getExitStatus() const { return (exit_status); }

const HttpRequest *CgiHandler::getRequest() const { return (request); }

time_t CgiHandler::getStartTime() const { return (start_time); }
