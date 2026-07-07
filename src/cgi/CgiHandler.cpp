#include "CgiHandler.hpp"
#include <sys/types.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <fcntl.h>
#include <sstream>

CgiHandler::CgiHandler(const HttpRequest* request, const char *body, size_t body_len)
	: pid(-1), exit_status(0), body(body), body_len(body_len), body_written(0), cgi_output(""),
	state(WRITING_BODY), request(request)
{
	pipe_in[0] = -1; pipe_in[1] = -1;
	pipe_out[0] = -1; pipe_out[1] = -1;
}

CgiHandler::~CgiHandler() 
{
	if (pipe_in[0] >= 0) close(pipe_in[0]);
	if (pipe_in[1] >= 0) close(pipe_in[1]);
	if (pipe_out[0] >= 0) close(pipe_out[0]);
	if (pipe_out[1] >= 0) close(pipe_out[1]);

	if (pid > 0 && state != CGI_DONE)
	{
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
	}
}

bool CgiHandler::start(const std::string& interpreter_path, const std::string& script_path,
				const std::string& server_name, const std::string& server_port)
{
	if (pipe(pipe_in) == -1) {state = CGI_ERROR; return (false);}
	if (pipe(pipe_out) == -1) {close(pipe_in[0]); close(pipe_in[1]); state = CGI_ERROR; return (false);}

	if (fcntl(pipe_in[0], F_SETFD, FD_CLOEXEC) == -1) {close(pipe_in[0]); close(pipe_in[1]); close(pipe_out[0]); close(pipe_out[1]); state = CGI_ERROR; return (false);}
	if (fcntl(pipe_in[1], F_SETFD, FD_CLOEXEC) == -1) {close(pipe_in[0]); close(pipe_in[1]); close(pipe_out[0]); close(pipe_out[1]); state = CGI_ERROR; return (false);}
	if (fcntl(pipe_out[0], F_SETFD, FD_CLOEXEC) == -1) {close(pipe_in[0]); close(pipe_in[1]); close(pipe_out[0]); close(pipe_out[1]); state = CGI_ERROR; return (false);}
	if (fcntl(pipe_out[1], F_SETFD, FD_CLOEXEC) == -1) {close(pipe_in[0]); close(pipe_in[1]); close(pipe_out[0]); close(pipe_out[1]); state = CGI_ERROR; return (false);}


	char *argv[3];
	argv[0] = const_cast<char *>(interpreter_path.c_str()); argv[1] = const_cast<char *>(script_path.c_str()); argv[2] = NULL;

	pid = fork();
	if (pid == -1) {close(pipe_in[0]); close(pipe_in[1]); close(pipe_out[0]); close(pipe_out[1]); state = CGI_ERROR; return (false);}
	else if (pid == 0)
	{
		close(pipe_in[1]); close(pipe_out[0]);

		if (dup2(pipe_in[0], STDIN_FILENO) == -1) _exit(1);
		if (dup2(pipe_out[1], STDOUT_FILENO) == -1) _exit(1);

		close(pipe_in[0]); close(pipe_out[1]);
	}
	else {}
}

// void CgiHandler::childProcess()
// {
// 	close(pipe_in[1]);
// 	close(pipe_out[0]);

// 	if (dup2(pipe_in[0], STDIN_FILENO) == -1)
// 	{
// 		std::cerr << "dup2()" << std::endl;
// 		_exit(1);
// 	}
// 	if (dup2(pipe_out[1], STDOUT_FILENO) == -1)
// 	{
// 		std::cerr << "dup2()" << std::endl;
// 		_exit(1);
// 	}

// 	close(pipe_in[0]);
// 	close(pipe_out[1]);

// 	size_t pos = _scriptPath.find_last_of('/');
// 	if (pos == std::string::npos)
// 		_exit(1);

// 	std::string dir = _scriptPath.substr(0, pos + 1);
// 	if (chdir(dir.c_str()) == -1)
// 	{
// 		std::cerr << "chdir()" << std::endl;
// 		_exit(1);
// 	}

// 	char *argv[] = {&_interpreter[0], &_scriptPath[0], NULL};
// 	char **env = buildEnvp();
// 	if (execve(argv[0], argv, env) == -1)
// 	{
// 		std::cerr << "execve" << std::endl;
// 		freeEnvp(env, _env.size() + 1);
// 		_exit(1);
// 	}
// }

// bool CgiHandler::readChunk()
// {
// 	int		status;
// 	char	buffer[1024];
// 	ssize_t	bytes;

// 	bytes = read(pipe_out[0], buffer, sizeof(buffer));
// 	if (bytes == 0)
// 	{
// 		waitpid(_pid, &status, 0);
// 		return (true);
// 	}
// 	else if (bytes > 0) 
// 	{
// 		_output.append(buffer, bytes);
// 		return (false);
// 	}
// 	else if (bytes == -1 && errno == EAGAIN)
// 		return (false);
	
// 	return (true);
// }

// int CgiHandler::start()
// {
// 	if (pipe(pipe_in) == -1)
// 		return (-1);
// 	if (pipe(pipe_out) == -1)
// 	{
// 		close(pipe_in[0]);
// 		close(pipe_in[1]);
// 		return (-1);
// 	}
// 	_pid = fork();
// 	if (_pid == -1)
// 	{
// 		close(pipe_in[0]);
// 		close(pipe_in[1]);
// 		close(pipe_out[0]);
// 		close(pipe_out[1]);
// 		return (-1);
// 	}
// 	else if (_pid == 0)
// 		childProcess();
// 	else
// 	{
// 		int status;

// 		close(pipe_in[0]);
// 		close(pipe_out[1]);

// 		if (!_body.empty())
// 		{
// 			if (write(pipe_in[1], &_body[0], _body.size()) == -1)
// 			{
// 				close(pipe_in[1]);
// 				waitpid(_pid, &status, 0);
// 				return (-1);
// 			}
// 		}
// 		close(pipe_in[1]);
// 	}
// 	return (pipe_out[0]);
// }

// std::string CgiHandler::buildResponse(const std::string &output)
// {
// 	size_t pos = output.find("\r\n\r\n");
// 	if (pos == std::string::npos)
// 		return ("HTTP/1.1 500 Internal Server Error\r\n\r\n");
// 	std::string headers = output.substr(0, pos);
// 	std::string body = output.substr(pos + 4);

// 	std::ostringstream oss;
// 	oss << body.size();
// 	std::string contentLength = oss.str();

// 	std::string response("HTTP/1.1 200 OK\r\n");
// 	response += headers + "\r\n";
// 	response += "Content-Length: " + contentLength + "\r\n";
// 	response += "\r\n" + body;

// 	return (response);
// }

// std::string CgiHandler::getOutput() const
// {
// 	return (_output);
// }