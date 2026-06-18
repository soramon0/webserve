#include "fsm.hpp"
#include "common.h"
#include "logger/log.hpp"
#include "request_state.hpp"

FSM::FSM()
    : req(NULL), state(stateStart), hasError(false), done(false),
      status(FSMStatus::OK) {
  req = new HttpRequest();
};

FSM::~FSM() {};

bool FSM::feedChunk(const char *buf, size_t len) {
  // TODO: grow arena
  if (!req->arena.setup(KIB(1))) {
    status = FSMStatus::OOM;
    return false;
  }

  Logger::debug("arena available space: %zu", req->arena.available());
  Logger::debug("socket recieved: %.*s", (int)len, buf);

  Context ctx(*this, req, buf, len, 0);

  while (ctx.offset < ctx.len) {
    state = state.next(ctx);

    if (ctx.hasError) {
      return false;
    }
  }

  return true;
}

bool FSM::finish() const { return status == FSMStatus::OK; }
