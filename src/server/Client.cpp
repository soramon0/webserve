#include "Client.hpp"
#include <string>
#include <sstream>
#include <cstdlib>

#define CRLF "\r\n"
#define BLANKLINE "\r\n\r\n"

Client::Client() : machine()
{
	addrlen = sizeof(addr);
	machine.setServer(this->srv);
}

Client::~Client() {}
