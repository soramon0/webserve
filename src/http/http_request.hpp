#pragma once

#include "lib/arena.hpp"
#include "lib/string_view.hpp"
#include "request_state.hpp"

typedef int STATUS;

class HttpRequest {
public:
  StringView method;
  StringView uri;
  StringView version;
  STATUS status;
  Arena *arena;

  HttpRequest();
  ~HttpRequest();

  void printRequest() const;
};
