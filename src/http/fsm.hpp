#pragma once

#include "http_request.hpp"
#include "request_state.hpp"
#include <cstddef>

class FSM {
private:
  HttpRequest *req;
  State state;
  bool hasError;
  bool done;

public:
  FSM();
  ~FSM();

  bool feedChunk(const char *buf, std::size_t len);
  bool finish() const;
  HttpRequest *getRequest() const { return req; };
};
