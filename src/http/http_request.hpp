#pragma once

#include "http/http_method.hpp"
#include "http/http_version.hpp"
#include "http/request_headers.hpp"
#include "http/status_code.hpp"
#include "lib/arena_list.hpp"
#include "lib/string_view.hpp"
#include "request_state.hpp"

class HttpRequest {
private:
  bool ready;
  size_t max_arena_blocks;

public:
  ArenaList arena;
  HttpStatus status;
  HttpMethod method;
  HttpVersion version;

  StringView method_view;
  StringView uri;
  StringView version_view;
  StringView error;

  Headers headers;

  HttpRequest();
  ~HttpRequest();

  void printRequest() const;
  bool updateField(StringView &field, const char *buf, size_t size);
  void dumpState();
  bool getStateReady() const { return ready; }
};
