#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>

class CgiHandler
{
private:
	int									pipe_in[2];
	int									pipe_out[2];
	pid_t									_pid;
	std::string							_output;
	std::string							_scriptPath;
	std::string							_interpreter;
	std::string							_body;
	std::map<std::string, std::string>	_env;

	void		childProcess();
	char**		buildEnvp() const;
	void		freeEnvp(char** envp, int size);

public:
	CgiHandler(const std::string& scriptPath,
			const std::string& interpreter,
			const std::string& body,
			const std::map<std::string, std::string>& env);

	int				start();
	bool			readChunk();
	std::string		getOutput() const;
	std::string		buildResponse(const std::string& cgiOutput);
};

#endif