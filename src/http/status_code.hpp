#pragma once

#include "lib/string_view.hpp"

// RFC HTTP response codes only
class HttpStatus {
public:
  enum Code {
    OK = 200,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    REQUEST_ENTITY_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504
  };

  HttpStatus() : value_(OK) {}
  explicit HttpStatus(Code v) : value_(v) {}
  explicit HttpStatus(int code) : value_(static_cast<Code>(code)) {}

  Code value() const { return value_; }
  int asInt() const { return static_cast<int>(value_); }
  bool isError() const { return asInt() >= 400; }

  HttpStatus &operator=(Code v) {
    value_ = v;
    return *this;
  }
  bool operator==(HttpStatus other) const { return value_ == other.value_; }
  bool operator==(Code v) const { return value_ == v; }
  bool operator!=(HttpStatus other) const { return value_ != other.value_; }
  bool operator!=(Code v) const { return value_ != v; }

  static StringView reasonPhrase(Code code) {
    switch (code) {
    case OK:
      return StringView("OK");
    case BAD_REQUEST:
      return StringView("Bad Request");
    case FORBIDDEN:
      return StringView("Forbidden");
    case NOT_FOUND:
      return StringView("Not Found");
    case METHOD_NOT_ALLOWED:
      return StringView("Method Not Allowed");
    case REQUEST_ENTITY_TOO_LARGE:
      return StringView("Request Entity Too Large");
    case URI_TOO_LONG:
      return StringView("URI Too Long");
    case REQUEST_HEADER_FIELDS_TOO_LARGE:
      return StringView("Request Header Fields Too Large");
    case INTERNAL_SERVER_ERROR:
      return StringView("Internal Server Error");
    case NOT_IMPLEMENTED:
      return StringView("Not Implemented");
    case BAD_GATEWAY:
      return StringView("Bad Gateway");
    case SERVICE_UNAVAILABLE:
      return StringView("Service Unavailable");
    case GATEWAY_TIMEOUT:
      return StringView("Gateway Timeout");
    default:
      return StringView("Internal Server Error");
    }
  }

private:
  Code value_;
};
