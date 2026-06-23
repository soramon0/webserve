#include "request-line.test.hpp"
#include "../utils.hpp"
#include "http/fsm.hpp"
#include "logger/log.hpp"

void testRequestLine(bool skip) {
  if (skip) {
    Logger::info("skipping testRequestLine...");
    return;
  }

  FSM fsm;
  std::string input = "GET /api/products HTTP/1.1\r\n";

  if (!fsm.feedChunk(input.c_str(), input.length())) {
    throw std::runtime_error("testRequestLine: parsing failed");
  }

  if (!fsm.status.isDone()) {
    Logger::error("Wanted: `%d` Got: `%d`", FSMStatus::DONE,
                  fsm.status.asInt());
    return;
  }

  HttpRequest *req = fsm.getRequest();

  if (req->status != HttpStatus::OK) {
    Logger::error("Wanted: `%d` Got: `%d`", HttpStatus::OK,
                  req->status.asInt());
    return;
  }
  assertStrEquals("GET", req->method.toString());
  assertStrEquals("/api/products",
                  std::string(req->uri.data(), req->uri.length()));
  assertStrEquals("HTTP/1.1", req->version.toString());
}
