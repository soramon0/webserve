#pragma once

#include "http/status_code.hpp"
#include "http_request.hpp"
#include "logger/log.hpp"
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
  int asInt() const { return static_cast<int>(value_); }
  const char *asStr() const {
    if (value_ == MALFORMED)
      return "MALFORMED";
    if (value_ == DONE)
      return "DONE";
    return "PENDING";
  };

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

public:
  FSMStatus status;
  StringView curr_header_key;
  StringView curr_header_value;

  FSM();
  ~FSM();

  bool feedChunk(const char *buf, std::size_t len);
  bool finish() const;
  HttpRequest *getRequest() const { return req; };

  void clear();
  void restart();
  void setDone();
  void setMalformed(HttpStatus::Code status, const char *msg);
  void setMalformed(HttpStatus::Code s, const StringView &error);
  void setMalformed500(const char *msg);
  void setMalformed400(const char *msg);

  void setMalformed(HttpStatus::Code status);
  void setMalformed500();
  void setMalformed400();
  bool consumeCRLF(const char *buf, size_t len, size_t &offset) const;
  bool isCRLF(unsigned char c) const { return c == '\r' || c == '\n'; }
  bool isOWS(unsigned char c) const { return c == ' ' || c == '\t'; }
  bool consumeOWS(const char *buf, size_t len, size_t &offset) const;

  void dumpState() const {
    Logger::info("fsm.status = %s", status.asStr());
    this->req->dumpState();
  }
};
