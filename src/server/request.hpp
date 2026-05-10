#pragma once

#include "../../include/common.h"
#include <iostream>
#include <string>
#include <map>

class Request
{
	//real request element
	std::string                         method;
	std::string                         path;
	std::string                         query;
	std::string                         protocol;
	std::map<std::string, std::string>  headers;
	std::string                         body;

	friend class Client;
	friend class Webserv;
	
public:
	Request();
	~Request();

	void printRequest() const;
};