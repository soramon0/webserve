#pragma once

#include <cstddef>

// Forward declration
class HttpRequest;
struct State;

struct Context {
  HttpRequest *req;
  const char *buf;
  size_t len;
  size_t offset;
  bool hasError;

  Context(HttpRequest *r, const char *b, size_t l, size_t o)
      : req(r), buf(b), len(l), offset(o), hasError(false) {};
};

typedef State (*StateFunction)(Context &ctx);

struct State {
  StateFunction next;
  State(StateFunction transition) : next(transition) {}
};

// Forward declration
State stateStart(Context &ctx);
State stateMethod(Context &ctx);
State stateURI(Context &ctx);
State stateVersion(Context &ctx);
State stateError(Context &ctx);
