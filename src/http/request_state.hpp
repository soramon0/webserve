#pragma once

#include "lib/string_view.hpp"

// TODO: remove. we're using state function
struct RequestState {
  enum Type {
    STATE_START,
    STATE_METHOD,
    STATE_URI,
    STATE_VERSION,
    STATE_FIRST_HEADER_KEY,
    STATE_HEADER_KEY,
    STATE_HEADER_VALUE,
    STATE_BODY_LENGTH,
    STATE_BODY_CHUNKED,
    STATE_DONE,
    STATE_MALFORMED,
    STATE_COUNT
  };

  Type value;

  RequestState() : value(STATE_START) {}
  RequestState(Type v) : value(v) {}

  // Allow implicit conversion to the underlying type for comparisons
  operator Type() const { return value; }
};

// Forward declration
struct State;
struct HttpRequest;

struct Context {
  HttpRequest &req;
  const StringView &buf;
  size_t &offset;

  Context(HttpRequest &r, const StringView &view, size_t &o)
      : req(r), buf(view), offset(o) {};
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
