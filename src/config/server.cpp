#include "server.hpp"
#include "redirect.hpp"
#include <sstream>

Server::Server() : port(8000), interface("0.0.0.0"), return_rule(NULL) {
  this->shared_config = new SharedConfig();
}

Server::Server(const Server &other)
    : port(other.port), interface(other.interface), locations(other.locations),
      return_rule(NULL) {
  if (other.shared_config) {
    this->shared_config = other.shared_config->clone();
  } else {
    this->shared_config = new SharedConfig();
  }

  if (other.return_rule) {
    this->return_rule = new ReturnDir(*other.return_rule);
  }
}

Server &Server::operator=(const Server &other) {
  if (this == &other) {
    return *this;
  }
  if (this->return_rule) {
    delete this->return_rule;
    this->return_rule = NULL;
  }
  if (this->shared_config) {
    delete this->shared_config;
    this->shared_config = NULL;
  }

  this->port = other.port;
  this->interface = other.interface;
  this->locations = other.locations;

  if (other.return_rule) {
    this->return_rule = new ReturnDir(*other.return_rule);
  }

  if (other.shared_config) {
    this->shared_config = other.shared_config->clone();
  }
  return *this;
}

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

Server &Server::withLocation(const std::string &path,
                             const Location &location) {
  Location loc(location);
  if (this->shared_config) {
    loc.withSharedConfig(*this->shared_config);
  }

  this->locations[path] = loc;
  return *this;
}

Server &Server::withSharedConfig(const SharedConfig &cfg) {
  if (this->shared_config) {
    delete this->shared_config;
  }

  this->shared_config = cfg.clone();

  for (std::map<std::string, Location>::iterator it = this->locations.begin();
       it != this->locations.end(); ++it) {
    it->second.withSharedConfig(*this->shared_config);
  }

  return *this;
}

std::string Server::toString(int indent) const {
  std::ostringstream oss;
  std::string tab = std::string(indent, '\t');

  oss << tab << "server {\n";
  oss << tab << "\t" << "listen " << this->interface << ":" << this->port
      << ";\n";

  if (this->return_rule) {
    oss << tab << "\t" << "return " << this->return_rule->code << " "
        << this->return_rule->url << ";\n";
  }
  for (std::map<std::string, Location>::const_iterator it =
           this->locations.begin();
       it != this->locations.end(); ++it) {
    oss << it->second.toString(indent + 1);
  }
  if (this->shared_config) {
    oss << this->shared_config->toString(indent + 1);
  }
  oss << tab << "}\n";
  return oss.str();
}
