#pragma once

#include <stdint.h>
#include <string>

struct ReturnDir {
  uint16_t code;   // optional, default to temporary redirect 302
  std::string url; // redirect url

  ReturnDir();
  ReturnDir(const ReturnDir &other);
  ReturnDir &operator=(const ReturnDir &other);

  ReturnDir(uint16_t code, const std::string url);
  ReturnDir(const std::string url);
};
