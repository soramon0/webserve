#include "CgiHandler.hpp"
#include <sys/types.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <sstream>

CgiHandler::CgiHandler(const std::string &scriptPath,
					   const std::string &interpreter,
					   const std::string &body,
					   const std::map<std::string, std::string> &env)
	: _scriptPath(scriptPath), _interpreter(interpreter), _body(body), _env(env)
{
	pipe_in[0] = -1;
	pipe_in[1] = -1;
	pipe_out[0] = -1;
	pipe_out[1] = -1;
}

char **CgiHandler::buildEnvp() const
{
	char **envp = new char *[_env.size() + 1];
	size_t i = 0;

	for (std::map<std::string, std::string>::const_iterator it = _env.begin();
		 it != _env.end(); it++)
	{
		std::string token = it->first + "=" + it->second;
		envp[i] = new char[token.size() + 1];
		std::memcpy(envp[i], token.c_str(), token.size() + 1);
		i++;
	}

	envp[i] = NULL;
	return (envp);
}

void CgiHandler::freeEnvp(char **envp, int size)
{
	for (int i = 0; i < size; i++)
		delete[] envp[i];
	delete[] envp;
}

void CgiHandler::childProcess()
{
		close(pipe_in[1]);
		close(pipe_out[0]);

		if (dup2(pipe_in[0], STDIN_FILENO) == -1)
		{
			std::cerr << "dup2()" << std::endl;
			_exit(1);
		}
		if (dup2(pipe_out[1], STDOUT_FILENO) == -1)
		{
			std::cerr << "dup2()" << std::endl;
			_exit(1);
		}
		close(pipe_in[0]);
		close(pipe_out[1]);

		size_t pos = _scriptPath.find_last_of('/');
		if (pos == std::string::npos)
			_exit(1);
		std::string dir = _scriptPath.substr(0, pos + 1);
		if (chdir(dir.c_str()) == - 1)
		{
			std::cerr << "chdir()" << std::endl;
			_exit(1);
		}

		char *argv[] = {&_interpreter[0], &_scriptPath[0], NULL};
		char **env = buildEnvp();
		if (execve(argv[0], argv, env) == -1)
		{
			std::cerr << "execve" << std::endl;
			freeEnvp(env, _env.size() + 1);
			_exit(1);
		}
}

std::string CgiHandler::parentProcess(pid_t pid)
{
	std::string output;
	int status;
		char buffer[1024];
		ssize_t bytes;

		close(pipe_in[0]);
		close(pipe_out[1]);

		if (!_body.empty())
		{
			if (write(pipe_in[1], &_body[0], _body.size()) == -1)
			{
				close(pipe_in[1]);
				waitpid(pid, &status, 0);
				return ("HTTP/1.1 500 Internal Server Error\r\n\r\n");
			}
		}
		close(pipe_in[1]);

		while ((bytes = read(pipe_out[0], buffer, sizeof(buffer))) > 0)
			output.append(buffer, bytes);
		waitpid(pid, &status, 0);
		return (output);
}

std::string CgiHandler::execute()
{
	std::string output;
	pid_t pid;

	if (pipe(pipe_in) == -1)
		return ("HTTP/1.1 500 Internal Server Error\r\n\r\n");
	if (pipe(pipe_out) == -1)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		return ("HTTP/1.1 500 Internal Server Error\r\n\r\n");
	}
	pid = fork();
	if (pid == -1)
	{
		close(pipe_in[0]);
		close(pipe_in[1]);
		close(pipe_out[0]);
		close(pipe_out[1]);
		return ("HTTP/1.1 500 Internal Server Error\r\n\r\n");
	}
	else if (pid == 0)
		childProcess();	
	else
		output = parentProcess(pid);
	return (output);
}

std::string CgiHandler::buildResponse(const std::string &output)
{
	size_t pos = output.find("\r\n\r\n");
	if (pos == std::string::npos)
		return ("HTTP/1.1 500 Internal Server Error\r\n\r\n");
	std::string headers = output.substr(0, pos);
	std::string body = output.substr(pos + 4);

	std::ostringstream oss;
	oss << body.size();
	std::string contentLength = oss.str();

	std::string response("HTTP/1.1 200 OK\r\n");
	response += headers + "\r\n";
	response += "Content-Length: " + contentLength + "\r\n";
	response += "\r\n" + body;

	return (response);
}