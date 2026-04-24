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

Server &Server::withSharedConfig(const SharedConfig &cfg) {
  if (this->shared_config) {
    delete this->shared_config;
  }

  this->shared_config = cfg.clone();
  return *this;
}

Server *Server::clone() {
  SharedConfig *cfg = NULL;
  if (this->shared_config) {
    cfg = this->shared_config->clone();
  }
  Server *clone = new Server(*this);
  clone->shared_config = cfg;

  return clone;
}
