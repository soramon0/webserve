#pragma once

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

  Context(HttpRequest &r) : req(r) {};
};

typedef State (*StateFunction)(const char *buf, Context &ctx);

struct State {
  StateFunction next;
  State(StateFunction transition) : next(transition) {}
};

// Forward declration
State stateStart(const char *buf, Context &ctx);
State stateMethod(const char *buf, Context &ctx);
State stateURI(const char *buf, Context &ctx);
State stateVersion(const char *buf, Context &ctx);


State stateStart(const char *buf, Context &ctx) {
  return stateMethod;
}
