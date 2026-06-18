#pragma once

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
    INTERNAL_SERVER_ERROR = 500,
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

private:
  Code value_;
};
