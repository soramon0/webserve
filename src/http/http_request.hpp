#pragma once

#include "lib/arena.hpp"
#include "lib/string_view.hpp"
#include "request_state.hpp"
#include <cstddef>

typedef int STATUS;

class HttpRequest {
  State state;
  Arena arena;

public:
  StringView method;
  StringView uri;
  StringView version;
  STATUS status;

  HttpRequest();
  ~HttpRequest();

  void printRequest() const;

  bool feedChunk(const char *buf, size_t len);
  bool finish() const;
};
