#ifndef CGIMANAGER_HPP
#define CGIMANAGER_HPP

#include "Request.hpp"
#include "CgiHandler.hpp"
#include <map>

class CgiManager
{
private:
	std::map<int, CgiHandler*>	_cgiHandlers;
	std::map<int, int>			_cgiToClient;
};

#endif