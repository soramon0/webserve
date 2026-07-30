#include "webserv_helper.hpp"
// #include "utils.hpp"
// #include "router.hpp"
// #include "cgi/CgiManager.hpp"
// #include "../http/status_code.hpp"
#include <cstring>
#include <iostream>
// #include <signal.h>
#include <sstream>
// #include <string>
// #include <sys/epoll.h>
// #include <sys/types.h>
// #include <map>

SOCKET createSocket(int id, Config& config)
{
	struct addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	std::ostringstream os;
	os << config.servers[id]->port;
	std::string port = os.str();
	std::string host = config.servers[id]->interface;

	struct addrinfo *addr;
	if (getaddrinfo(host.c_str(), port.c_str(), &hints, &addr)) {
        throw std::runtime_error("getaddrinfo failed for " + host + ":" + port);
	}

	int socket_listen =
		socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (!ISVALIDSOCKET(socket_listen)) {
        freeaddrinfo(addr);
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));
    }

	int opt = 1;
	if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR, &opt,
				   sizeof(opt)))
		Logger::error("setsockopt failed");

	if (bind(socket_listen, addr->ai_addr, addr->ai_addrlen)) {
        freeaddrinfo(addr);
        close(socket_listen);
        throw std::runtime_error("bind() failed on " + host + ":" + port + ": " + strerror(errno));
    }
	freeaddrinfo(addr);

	Logger::info("Listening on http://%s:%s ...", host.c_str(), port.c_str());
	if (listen(socket_listen, SOMAXCONN)) {
        close(socket_listen);
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
    }

	return socket_listen;
}
