#include "server.hpp"

Server::Server() : port(8000), interface("0.0.0.0"), return_rule(NULL) {}

Server::~Server() {
  if (this->return_rule) {
    delete this->return_rule;
  }

  if (this->shared_config) {
    delete this->shared_config;
  }
};

Server &Server::withPort(uint16_t port) {
  this->port = port;
  return *this;
}

Server &Server::withInterface(const std::string &interface) {
  this->interface = interface;
  return *this;
}

Server &Server::withRedirect(uint16_t code, const std::string &url) {
  if (this->return_rule) {
    delete this->return_rule;
  }

  this->return_rule = new ReturnDir(code, url);
  return *this;
}

Server &Server::withRedirect(const std::string &url) {
  if (this->return_rule) {
    delete this->return_rule;
  }

  this->return_rule = new ReturnDir(url);
  return *this;
}

Server &Server::withLocation(const std::string &path, const Location &loc) {
  this->locations[path] = loc;
  return *this;
}

void Server::assignSharedConfig(SharedConfig *shared_config) {
  if (this->shared_config) {
    delete shared_config;
  }
  this->shared_config = shared_config;
}
