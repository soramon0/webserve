#pragma once
#include "location.hpp"
#include "redirect.hpp"
#include <map>
#include <string>

class Server {
  // std::string server_name; virtual hosts
  int port;
  std::string interface;
  std::map<std::string, Location> locations;
  ReturnDir *return_rule;

  // copy from parent (http) and update if needed to match server
  SharedConfig *shared_config;
};
