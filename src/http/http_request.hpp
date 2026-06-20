#pragma once

#include "http/status_code.hpp"
#include "lib/arena.hpp"
#include "lib/string_view.hpp"
#include "request_state.hpp"

class HttpRequest {
public:
  Arena arena;
  HttpStatus status;

  StringView method;
  StringView uri;
  StringView version;

  HttpRequest();
  ~HttpRequest();

  void printRequest() const;
};
