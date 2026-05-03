#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>

class CgiHandler
{
private:
	int									pipe_in[2];
	int									pipe_out[2];
	std::string							_scriptPath;
	std::string							_interpreter;
	std::string							_body;
	std::map<std::string, std::string>	_env;

	std::string	parentProcess(pid_t pid);
	void		childProcess();

	char**		buildEnvp() const;
	void		freeEnvp(char** envp, int size);

public:
	CgiHandler(const std::string& scriptPath,
			const std::string& interpreter,
			const std::string& body,
			const std::map<std::string, std::string>& env);

	std::string	execute();
	std::string	buildResponse(const std::string& cgiOutput);
};

#endif