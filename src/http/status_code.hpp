#pragma once

// RFC HTTP response codes only
class HttpStatus {
public:
  enum Code {
    OK = 200,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    REQUEST_TIMEOUT = 408,
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
  const char* toString() const {
    switch (value_) {
      case OK:                            return "200 OK";
      case MOVED_PERMANENTLY:             return "301 Moved Permanently";
      case FOUND:                         return "302 Found";
      case TEMPORARY_REDIRECT:            return "307 Temporary Redirect";
      case PERMANENT_REDIRECT:            return "308 Permanent Redirect";
      case BAD_REQUEST:                   return "400 Bad Request";
      case FORBIDDEN:                     return "403 Forbidden";
      case NOT_FOUND:                     return "404 Not Found";
      case METHOD_NOT_ALLOWED:            return "405 Method Not Allowed";
      case REQUEST_ENTITY_TOO_LARGE:      return "413 Content Too Large";
      case URI_TOO_LONG:                  return "414 URI Too Long";
      case REQUEST_HEADER_FIELDS_TOO_LARGE: return "431 Request Header Fields Too Large";
      case INTERNAL_SERVER_ERROR:         return "500 Internal Server Error";
      case NOT_IMPLEMENTED:               return "501 Not Implemented";
      case BAD_GATEWAY:                   return "502 Bad Gateway";
      case SERVICE_UNAVAILABLE:           return "503 Service Unavailable";
      case GATEWAY_TIMEOUT:               return "504 Gateway Timeout";
      default:                            return "500 Internal Server Error";
    }
}
  HttpStatus &operator=(Code v) {
    value_ = v;
    return *this;
  }
  bool operator==(HttpStatus other) const { return value_ == other.value_; }
  bool operator>=(HttpStatus other) const { return value_ >= other.value_; }
  bool operator==(Code v) const { return value_ == v; }
  bool operator>=(Code v) const { return value_ >= v; }
  bool operator!=(HttpStatus other) const { return value_ != other.value_; }
  bool operator!=(Code v) const { return value_ != v; }

private:
  Code value_;
};
