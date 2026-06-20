#pragma once

#include "http/status_code.hpp"
#include "http_request.hpp"
#include "request_state.hpp"
#include <cstddef>

class FSMStatus {
public:
  enum Code {
    PENDING = 0,
    MALFORMED,
    DONE,
  };

  FSMStatus() : value_(PENDING) {}
  explicit FSMStatus(Code v) : value_(v) {}

  Code value() const { return value_; }
  bool isPending() const { return value_ == PENDING; }
  bool isMalformed() const { return value_ == MALFORMED; }
  bool isDone() const { return value_ == DONE; }

  FSMStatus &operator=(Code v) {
    value_ = v;
    return *this;
  }
  bool operator==(FSMStatus other) const { return value_ == other.value_; }
  bool operator==(Code v) const { return value_ == v; }
  bool operator!=(FSMStatus other) const { return value_ != other.value_; }
  bool operator!=(Code v) const { return value_ != v; }

private:
  Code value_;
};

class FSM {
private:
  HttpRequest *req;
  State state;
  bool hasError;
  bool done;

public:
  FSMStatus status;

  FSM();
  ~FSM();

  bool feedChunk(const char *buf, std::size_t len);
  bool finish() const;
  HttpRequest *getRequest() const { return req; };

  void setMalformed(const HttpStatus status);
  void setMalformed500();
  void setMalformed400();
};
