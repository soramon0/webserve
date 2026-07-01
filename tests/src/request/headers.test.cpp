#include "doctest.h"
#include "http/fsm.hpp"

TEST_CASE("headers not required in http/1.0") {
  FSM fsm;
  std::string input = "GET / HTTP/1.0\r\n\r\n";

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(req->headers.size() == 0);
}

TEST_CASE("headers saved normalized") {
  FSM fsm;
  std::string input = "GET / HTTP/1.0\r\n"
                      "Host:   \texample.com   \t\t\n\n";

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(req->headers.size() == 1);
  const StringView *host = req->headers.get("host");
  REQUIRE(host != nullptr);
  CHECK(*host == StringView("example.com"));
}
