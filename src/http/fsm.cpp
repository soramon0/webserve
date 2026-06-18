#include "fsm.hpp"
#include "common.h"
#include "logger/log.hpp"
#include "request_state.hpp"

FSM::FSM() : req(NULL), state(stateStart), hasError(false), done(false) {
  req = new HttpRequest();
};

FSM::~FSM() {};

bool FSM::feedChunk(const char *buf, size_t len) {
  // TODO: grow arena
  if (!req->arena.setup(KIB(1))) {
    req->status = 1; // OOM
    return false;
  }

  Logger::debug("arena available space: %zu", req->arena.available());
  Logger::debug("socket recieved: %.*s", (int)len, buf);

  Context ctx(req, buf, len, 0);

  while (ctx.offset < ctx.len) {
    state = state.next(ctx);

    if (ctx.hasError) {
      return false;
    }
  }

  return true;
}

bool FSM::finish() const {
  // find indicator for done
  return done;
}
