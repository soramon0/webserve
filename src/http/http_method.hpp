#pragma once

#include "lib/string_view.hpp"
#include <cstring>
#include <ostream>
#include <string>

class HttpMethod {
public:
  enum Method { GET, POST, PUT, DELETE, HEAD, OPTIONS, UNKNOWN };

  HttpMethod() : value_(UNKNOWN) {};
  explicit HttpMethod(Method v) : value_(v) {};
  explicit HttpMethod(int v) : value_(static_cast<Method>(v)) {};
  explicit HttpMethod(const char *method) {
    parse(method, std::strlen(method));
  }
  explicit HttpMethod(const std::string &method) {
    parse(method.c_str(), method.length());
  }
  explicit HttpMethod(const StringView &method) {
    parse(method.data(), method.length());
  }

  Method value() const { return value_; }
  bool isUnknown() const { return value_ == UNKNOWN; }
  bool isSupported() const {
    return value_ == GET || value_ == POST || value_ == DELETE;
  }

  HttpMethod &operator=(Method v) {
    value_ = v;
    return *this;
  }
  bool operator==(HttpMethod other) const { return value_ == other.value_; }
  bool operator==(Method v) const { return value_ == v; }
  bool operator!=(HttpMethod other) const { return value_ != other.value_; }
  bool operator!=(Method v) const { return value_ != v; }

  std::string toString() const {
    switch (value_) {
    case GET:
      return "GET";
    case POST:
      return "POST";
    case PUT:
      return "PUT";
    case DELETE:
      return "DELETE";
    case HEAD:
      return "HEAD";
    case OPTIONS:
      return "OPTIONS";
    default:
      return "UNKNOWN";
    }
  }

private:
  Method value_;

  void parse(const char *method, size_t len) {
    char buf[8] = {0};

    if (!method || len == 0 || len > 7) {
      value_ = UNKNOWN;
      return;
    }
    std::memcpy(buf, method, len);
    buf[len] = '\0';

    // RFC 7230: method tokens are case-sensitive
    if (std::strcmp(buf, "GET") == 0) {
      value_ = GET;
    } else if (std::strcmp(buf, "POST") == 0) {
      value_ = POST;
    } else if (std::strcmp(buf, "DELETE") == 0) {
      value_ = DELETE;
    } else if (std::strcmp(buf, "PUT") == 0) {
      value_ = PUT;
    } else if (std::strcmp(buf, "HEAD") == 0) {
      value_ = HEAD;
    } else if (std::strcmp(buf, "OPTIONS") == 0) {
      value_ = OPTIONS;
    } else {
      value_ = UNKNOWN;
    }
  }
};

inline std::ostream &operator<<(std::ostream &o, HttpMethod method) {
  o << method.toString();
  return o;
}
