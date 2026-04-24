#pragma once

#include "redirect.hpp"
#include "shared_config.hpp"
#include <string>

class Location {
private:
  std::string path;
  ReturnDir *return_rule;

  // copy from parent (server) and update if needed to match location
  SharedConfig *shared_config;
  friend class Server;

public:
  Location();
  ~Location();
  Location(const Location &other);
  Location &operator=(const Location &other);
  Location(const std::string &path);

  Location &withPath(const std::string &path);
  Location &withRedirect(uint16_t code, const std::string &url);
  Location &withRedirect(const std::string &url);
  Location &withSharedConfig(const SharedConfig &cfg);

  std::string toString(int indent) const;
};
