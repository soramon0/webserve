#include "config.hpp"
#include <sstream>

Config::Config() : shared_config(NULL) {};

Config::~Config() { delete shared_config; }

void Config::assignSharedConfig(SharedConfig *shared_config) {
  if (this->shared_config) {
    delete this->shared_config;
  }

  this->shared_config = shared_config;
}

std::string Config::toString() const {
  std::ostringstream oss;

  oss << "http {\n" << this->shared_config->toString(1) << "}";
  return oss.str();
}
