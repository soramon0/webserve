#pragma once

#include "lib/string_view.hpp"
#include <cctype>
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
    for (size_t i = 0; i < len; i++) {
      buf[i] = std::tolower(static_cast<unsigned char>(buf[i]));
    }

    if (std::strcmp(buf, "get") == 0) {
      value_ = GET;
    } else if (std::strcmp(buf, "post") == 0) {
      value_ = POST;
    } else if (std::strcmp(buf, "delete") == 0) {
      value_ = DELETE;
    } else if (std::strcmp(buf, "put") == 0) {
      value_ = PUT;
    } else if (std::strcmp(buf, "head") == 0) {
      value_ = HEAD;
    } else if (std::strcmp(buf, "options") == 0) {
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
