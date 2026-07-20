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
	  machine(),
	  cgiManager(NULL),
	  cgi_pending(false),
	  response_from_cgi(false),
	  cgiResponse(),
	  last_activity(0),
	  request_start(0)
{
	std::memset(&addr, 0, sizeof(addr));
	machine.setServer(this->srv);
}

Client::~Client() {}
