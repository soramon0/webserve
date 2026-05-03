#include "CgiHandler.hpp"
#include <iostream>

int main()
{
	std::map<std::string, std::string> env;

	env["REQUEST_METHOD"] = "POST";
	env["CONTENT_TYPE"] = "application/x-www-form-urlencoded";
	env["QUERY_STRING"] = "name=asma";

	CgiHandler cgi("./test.py", "/usr/bin/python3", "name=asma", env);
	std::cout << cgi.buildResponse(cgi.execute()) << std::endl;


	return (0);
}