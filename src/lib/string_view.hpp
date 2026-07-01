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
  void clear() {
    buf = NULL;
    len = 0;
  }

  char operator[](std::size_t index) const { return buf[index]; }

  const char *startsWith(const char *target) const;
  const char *endsWith(const char *target) const;
};

inline bool operator<(const StringView &lhs, const StringView &rhs) {
  std::size_t min_len =
      (lhs.length() < rhs.length()) ? lhs.length() : rhs.length();
  int cmp = std::strncmp(lhs.data(), rhs.data(), min_len);

  if (cmp != 0) {
    return cmp < 0;
  }
  return lhs.length() < rhs.length();
}

inline bool operator==(const StringView &lhs, const StringView &rhs) {
  if (lhs.length() != rhs.length()) {
    return false;
  }

  if (lhs.data() == rhs.data()) {
    return true;
  }

  return std::strncmp(lhs.data(), rhs.data(), lhs.length()) == 0;
}

inline bool operator==(const StringView &lhs, const char *rhs) {
  return lhs == StringView(rhs);
}

inline bool operator==(const char *lhs, const StringView &rhs) {
  return StringView(lhs) == rhs;
}

inline std::ostream &operator<<(std::ostream &os, const StringView &sv) {
  if (sv.data()) {
    os.write(sv.data(), sv.length());
  }

  return os;
}
