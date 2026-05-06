#include "CgiHandler.hpp"
#include <iostream>
#include <fcntl.h>

int main()
{
	std::map<std::string, std::string> env;

	env["REQUEST_METHOD"] = "POST";
	env["CONTENT_TYPE"] = "application/x-www-form-urlencoded";
	env["QUERY_STRING"] = "name=asma";

	CgiHandler cgi("./test.py", "/usr/bin/python3", "name=asma", env);
	int fd = cgi.start();
	if (fd == -1)
	{
		std::cerr << "strat() failed" << std::endl;
		return (1);
	}
	while (!cgi.readChunk());
	std::cout << cgi.buildResponse(cgi.getOutput()) << std::endl;


	return (0);
}