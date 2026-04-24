

#include "location.hpp"
#include "redirect.hpp"

Location::Location() : path(), return_rule(NULL) {};

Location::Location(const std::string &path) : path(path), return_rule(NULL) {};

Location::~Location() {
  if (this->return_rule) {
    delete this->return_rule;
  }

  if (this->shared_config) {
    delete this->shared_config;
  }
};

Location &Location::withPath(const std::string &path) {
  this->path = path;
  return *this;
}

Location &Location::withRedirect(uint16_t code, const std::string &url) {
  if (this->return_rule) {
    delete this->return_rule;
  }

  this->return_rule = new ReturnDir(code, url);
  return *this;
}

Location &Location::withRedirect(const std::string &url) {
  if (this->return_rule) {
    delete this->return_rule;
  }

  this->return_rule = new ReturnDir(url);
  return *this;
}

Location &Location::withSharedConfig(const SharedConfig &cfg) {
  if (this->shared_config) {
    delete this->shared_config;
  }

  this->shared_config = cfg.clone();
  return *this;
}
