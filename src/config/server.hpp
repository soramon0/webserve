#pragma once
#include "location.hpp"
#include "redirect.hpp"
#include <map>
#include <stdint.h>
#include <string>

class Server {
private:
  uint16_t port;
  std::string interface;
  std::map<std::string, Location> locations;
  ReturnDir *return_rule;

  // copy from parent (http) and update if needed to match server
  SharedConfig *shared_config;

  friend class Config;

public:
  Server();
  virtual ~Server();

  Server &withPort(uint16_t port);
  Server &withInterface(const std::string &interface);
  Server &withRedirect(uint16_t code, const std::string &url);
  Server &withRedirect(const std::string &url);
  Server &withLocation(const std::string &path, const Location &loc);
  Server &withSharedConfig(const SharedConfig &cfg);
  Server *clone();
};
