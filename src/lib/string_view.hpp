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
  bool operator==(const StringView &other) const;
  bool operator<(const StringView &other) const;

  const char *startsWith(const char *target) const;
  const char *endsWith(const char *target) const;
};

inline std::ostream &operator<<(std::ostream &os, const StringView &sv) {
  if (sv.data()) {
    os.write(sv.data(), sv.length());
  }

  return os;
}
