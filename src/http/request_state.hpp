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

// Forward declration
State stateStart(Context &ctx);
State stateMethod(Context &ctx);
State stateURI(Context &ctx);
State stateVersion(Context &ctx);
State stateHeaderKey(Context &ctx);
State stateHeaderValue(Context &ctx);
State stateBody(Context &ctx);
State stateBodyChunked(Context &ctx);
State stateDone(Context &ctx);
State stateError(Context &ctx);

struct State {
  StateFunction next;
  State(StateFunction transition) : next(transition) {}

  enum Progression {
    START = 0,
    METHOD,
    URI,
    VERSION,
    HEADER_KEY,
    HEADER_VALUE,
    BODY,
    DONE,
    ERROR,
    COUNT,
    UNKNOWN,
  };

  struct Mapping {
    StateFunction fn;
    Progression progression;
  };

  Progression getProgression(StateFunction func) {
    static const Mapping table[] = {{stateStart, START},
                                    {stateMethod, METHOD},
                                    {stateURI, URI},
                                    {stateVersion, VERSION},
                                    {stateHeaderKey, HEADER_KEY},
                                    {stateHeaderValue, HEADER_VALUE},
                                    {stateBody, BODY},
                                    {stateDone, DONE},
                                    {stateError, ERROR}};

    size_t tableSize = sizeof(table) / sizeof(table[0]);

    for (size_t i = 0; i < tableSize; ++i) {
      if (table[i].fn == func) {
        return table[i].progression;
      }
    }
    return UNKNOWN;
  }
};
