#include "config.hpp"
#include <sstream>

Config::Config() { this->shared_config = new SharedConfig(); };

Config::~Config() { delete shared_config; }

Config &Config::withServer(const Server &server) {
  Server srv(server);

  if (this->shared_config) {
    srv.withSharedConfig(*this->shared_config);
  }

  this->servers.push_back(srv);
  return *this;
}

Config &Config::withSharedConfig(const SharedConfig &cfg) {
  if (this->shared_config) {
    delete this->shared_config;
  }

  this->shared_config = cfg.clone();
  return *this;
}

bool Config::hasServer(const Server &srv) const {
  if (servers.empty())
    return false;

  std::vector<Server>::const_iterator it;
  for (it = servers.begin(); it != servers.end(); it++) {
    if (it->interface == srv.interface) {
      return true;
    }
  }
  return false;
}

std::string Config::toString() const {
  std::ostringstream oss;

  oss << "http {\n";

  if (this->shared_config) {
    oss << this->shared_config->toString(1);
  }

  if (this->servers.size() > 0) {
    for (std::vector<Server>::const_iterator it = this->servers.begin();
         it != this->servers.end(); ++it) {
      oss << it->toString(1);
    }
  }

  oss << "}";
  return oss.str();
}
