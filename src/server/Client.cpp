#include "Client.hpp"
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>

Client::Client()
	: addrlen(0),
	  socket(-1),
	  srv(NULL),
	  location(NULL),
	  machine()
{
	std::memset(&addr, 0, sizeof(addr));
	machine.setServer(this->srv);
}

Client::~Client() {}
