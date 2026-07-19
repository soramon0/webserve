#pragma once

#include "lib/string_view.hpp"
#include "redirect.hpp"
#include "shared_config.hpp"
#include <string>
#include <vector>

struct Location {
  std::string path;
  ReturnDir *return_rule;
  // copy from parent (server) and update if needed to match location
  SharedConfig *shared_config;
  std::vector<std::string> methods;

  Location();
  ~Location();
  Location(const Location &other);
  Location &operator=(const Location &other);
  Location(const std::string &path);

  Location &withPath(const std::string &path);
  Location &withMethod(const std::string &method);
  Location &withRedirect(uint16_t code, const std::string &url);
  Location &withRedirect(const std::string &url);
  Location &withSharedConfig(const SharedConfig &cfg);

  bool hasMethod(const StringView &method) const;
  std::string toString(int indent) const;
};
