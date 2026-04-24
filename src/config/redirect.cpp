
#include "redirect.hpp"

ReturnDir::ReturnDir(uint16_t code, const std::string url)
    : code(code), url(url) {}

ReturnDir::ReturnDir(const std::string url) : code(302), url(url) {}
