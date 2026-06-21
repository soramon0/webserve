#pragma once

#include <cstddef>

// Forward declration
class HttpRequest;
class FSM;
struct State;

struct Context {
  FSM &fsm;
  HttpRequest *req;
  const char *buf;
  size_t len;
  size_t offset;

  Context(FSM &f, HttpRequest *r, const char *b, size_t l, size_t o)
      : fsm(f), req(r), buf(b), len(l), offset(o) {};
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
