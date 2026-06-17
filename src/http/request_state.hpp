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
