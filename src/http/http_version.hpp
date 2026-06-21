#pragma once

#include "lib/string_view.hpp"
#include <cstring>

class HttpVersion {
public:
  enum Version { V1_0, V1_1, UNKNOWN };

  HttpVersion() : value_(UNKNOWN) {}
  explicit HttpVersion(Version v) : value_(v) {}
  explicit HttpVersion(const StringView &version) {
    parse(version.data(), version.length());
  }

  Version value() const { return value_; }
  bool isUnknown() const { return value_ == UNKNOWN; }
  bool isSupported() const { return value_ == V1_0 || value_ == V1_1; }

  bool operator==(HttpVersion other) const { return value_ == other.value_; }
  bool operator==(Version v) const { return value_ == v; }
  bool operator!=(HttpVersion other) const { return value_ != other.value_; }
  bool operator!=(Version v) const { return value_ != v; }

private:
  Version value_;

  void parse(const char *data, size_t len) {
    if (!data || len == 0) {
      value_ = UNKNOWN;
      return;
    }

    if (len == 8 && std::memcmp(data, "HTTP/1.1", 8) == 0) {
      value_ = V1_1;
    } else if (len == 8 && std::memcmp(data, "HTTP/1.0", 8) == 0) {
      value_ = V1_0;
    } else {
      value_ = UNKNOWN;
    }
  }
};
