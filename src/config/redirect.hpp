#pragma once

#include <stdint.h>
#include <string>

class ReturnDir {
private:
  uint16_t code;   // optional, default to temporary redirect 302
  std::string url; // redirect url

public:
  ReturnDir(uint16_t code, const std::string url);
  ReturnDir(const std::string url);
};
