#pragma once

#include <cstddef>
#include <cstring>
#include <iostream>

class StringView {
private:
  const char *buf;
  std::size_t len;

public:
  StringView() : buf(NULL), len(0) {};
  StringView(const char *str, std::size_t len) : buf(str), len(len) {};
  StringView(const char *str) : buf(str), len(str ? std::strlen(str) : 0) {}

  const char *data() const { return buf; }
  std::size_t length() const { return len; }
  bool empty() const { return len == 0; }

  char operator[](std::size_t index) const { return buf[index]; }
};

inline std::ostream &operator<<(std::ostream &os, const StringView &sv) {
  for (std::size_t i = 0; i < sv.length(); ++i) {
    os << sv[i];
  }
  return os;
}
