#include "config.hpp"
#include <sstream>

Config::Config() : shared_config(NULL) {};

Config::~Config() {
  delete shared_config;

  for (std::vector<Server *>::iterator it = servers.begin();
       it != servers.end(); ++it) {
    delete *it;
  }
}

Config &Config::withServer(const Server &server) {
  Server srv = server;
  this->servers.push_back(srv.clone());
  return *this;
}

Config &Config::withSharedConfig(const SharedConfig &cfg) {
  if (this->shared_config) {
    delete this->shared_config;
  }

  this->shared_config = cfg.clone();
  return *this;
}

std::string Config::toString() const {
  std::ostringstream oss;

  oss << "http {\n" << this->shared_config->toString(1) << "}";
  return oss.str();
}
