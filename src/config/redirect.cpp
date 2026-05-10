
#include "redirect.hpp"

ReturnDir::ReturnDir() : code(302), url() {}

ReturnDir::ReturnDir(const ReturnDir &other)
    : code(other.code), url(other.url) {}

ReturnDir &ReturnDir::operator=(const ReturnDir &other) {
  if (this == &other) {
    return *this;
  }
  this->code = other.code;
  this->url = other.url;
  return *this;
}

ReturnDir::ReturnDir(uint16_t code, const std::string url)
    : code(code), url(url) {}

ReturnDir::ReturnDir(const std::string url) : code(302), url(url) {}
