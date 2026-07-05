#include "fsm.hpp"
#include "logger/log.hpp"
#include "request_state.hpp"

FSM::FSM() : req(NULL), state(stateStart), status(FSMStatus::PENDING) {
  req = new HttpRequest();
}

FSM::~FSM() { clear(); }

void FSM::clear() {
  if (req) {
    delete req;
    req = NULL;
  }
  state = stateStart;
  status = FSMStatus::PENDING;
  curr_header_value.clear();
  curr_header_key.clear();
}

void FSM::restart() {
  clear();
  req = new HttpRequest();
}

bool FSM::feedChunk(const char *buf, size_t len) {
  if (!req->getStateReady()) {
    setMalformed500();
    return false;
  }

  Logger::debug("socket recieved: %.*s", (int)len, buf);

  if (len == 0) {
    this->setMalformed400();
    return false;
  }

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

void FSM::setMalformed(HttpStatus::Code s, const StringView &error) {
  status = FSMStatus::MALFORMED;
  this->req->status = s;
  this->req->error = error;
}

void FSM::setMalformed(HttpStatus::Code s, const char *msg) {
  status = FSMStatus::MALFORMED;
  this->req->status = s;
  if (msg) {
    this->req->error = StringView(msg);
  }
}

void FSM::setMalformed(HttpStatus::Code s) { setMalformed(s, NULL); }

void FSM::setMalformed500(const char *msg) {
  status = FSMStatus::MALFORMED;

  if (msg) {
    this->req->error = StringView(msg);
  } else {
    this->req->error = StringView("internal server error");
  }
  this->req->status = HttpStatus::INTERNAL_SERVER_ERROR;
}

void FSM::setMalformed500() { setMalformed500(NULL); }

void FSM::setMalformed400(const char *msg) {
  status = FSMStatus::MALFORMED;
  this->req->status = HttpStatus::BAD_REQUEST;
  if (msg) {
    this->req->error = StringView(msg);
  } else {
    this->req->error = StringView("bad request");
  }
}

void FSM::setMalformed400() { setMalformed400(NULL); }

bool FSM::consumeCRLF(const char *buf, size_t len, size_t &offset) const {
  if (offset >= len) {
    return false;
  }

  if (buf[offset] == '\r') {
    offset++;
    if (offset >= len) {
      // buffer too small
      return false;
    }
    if (buf[offset] == '\n') {
      offset++;
      return true;
    }
  } else if (buf[offset] == '\n') {
    offset++;
    return true;
  }
  return false;
}

bool FSM::consumeOWS(const char *buf, size_t len, size_t &offset) const {
  while (offset < len && isOWS(buf[offset])) {
    offset++;
  }

  return true;
}
