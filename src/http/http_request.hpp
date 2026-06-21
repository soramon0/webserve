#pragma once

#include "http/http_method.hpp"
#include "http/status_code.hpp"
#include "lib/arena.hpp"
#include "lib/string_view.hpp"
#include "request_state.hpp"

class HttpRequest {
public:
  Arena arena;
  HttpStatus status;
  HttpMethod method;

  StringView method_view;
  StringView uri;
  StringView version;


  HttpRequest();
  ~HttpRequest();

  void printRequest() const;
  bool updateField(StringView &field, const char *buf, size_t size);
};
