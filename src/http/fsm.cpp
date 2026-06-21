#include "fsm.hpp"
#include "common.h"
#include "logger/log.hpp"
#include "request_state.hpp"

FSM::FSM() : req(NULL), state(stateStart), status(FSMStatus::PENDING) {
  req = new HttpRequest();
};

FSM::~FSM() {};

bool FSM::feedChunk(const char *buf, size_t len) {
  // TODO: grow arena
  if (!req->arena.setup(KIB(1))) {
    setMalformed500();
    return false;
  }

  Logger::debug("arena available space: %zu", req->arena.available());
  Logger::debug("socket recieved: %.*s", (int)len, buf);

  Context ctx(*this, req, buf, len, 0);

  while (ctx.offset < ctx.len) {
    if (finish()) {
      break;
    }

    state = state.next(ctx);
  }

  return !status.isMalformed();
}

bool FSM::finish() const { return status.isDone() || status.isMalformed(); }

void FSM::setDone() { status = FSMStatus::DONE; }

void FSM::setMalformed(HttpStatus::Code s) {
  status = FSMStatus::MALFORMED;
  this->req->status = s;
}

void FSM::setMalformed500() {
  status = FSMStatus::MALFORMED;
  this->req->status = HttpStatus::INTERNAL_SERVER_ERROR;
}

void FSM::setMalformed400() {
  status = FSMStatus::MALFORMED;
  this->req->status = HttpStatus::BAD_REQUEST;
}
