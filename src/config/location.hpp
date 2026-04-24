#pragma once

#include "redirect.hpp"
#include "shared_config.hpp"
#include <string>

class Location {
  std::string path;
  ReturnDir *return_rule;

  // copy from parent (server) and update if needed to match location
  SharedConfig *shared_config;
};
