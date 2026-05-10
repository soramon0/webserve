#include "request.hpp"

Request::Request() {}
Request::~Request() {}

void Request::printRequest() const
{
    std::cout << "----- REQUEST -----" << std::endl;

    std::cout << "Method   : " << method << std::endl;
    std::cout << "Path     : " << path << std::endl;
    std::cout << "Query    : " << query << std::endl;
    std::cout << "Protocol : " << protocol << std::endl;

    std::cout << "\nHeaders:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        std::cout << it->first << ": " << it->second << std::endl;
    }

    std::cout << "\nBody:" << std::endl;
    std::cout << body << std::endl;

    std::cout << "-------------------" << std::endl;

}