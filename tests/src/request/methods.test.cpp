#include "doctest.h"
#include "http/fsm.hpp"
#include <string>

TEST_CASE("FSM accepts supported HTTP methods") {
  FSM fsm;

  SUBCASE("POST") {
    std::string input = "POST / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    CHECK(fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isDone());

    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    CHECK(req->status == HttpStatus::OK);
    CHECK(req->method == HttpMethod::POST);
  }

  SUBCASE("DELETE") {
    std::string input = "DELETE /item HTTP/1.1\r\nHost: localhost\r\n\r\n";
    CHECK(fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isDone());

    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    CHECK(req->status == HttpStatus::OK);
    CHECK(req->method == HttpMethod::DELETE);
  }
}

TEST_CASE("FSM rejects unsupported but recognized HTTP methods") {
  FSM fsm;
  std::string input;

  SUBCASE("PUT") {
    input = "PUT / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    CHECK(!fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isMalformed());

    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    CHECK(req->status == HttpStatus::METHOD_NOT_ALLOWED);
  }

  SUBCASE("HEAD") {
    input = "HEAD / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    CHECK(!fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isMalformed());

    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    CHECK(req->status == HttpStatus::METHOD_NOT_ALLOWED);
  }

  SUBCASE("OPTIONS") {
    input = "OPTIONS / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    CHECK(!fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isMalformed());

    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    CHECK(req->status == HttpStatus::METHOD_NOT_ALLOWED);
  }
}

TEST_CASE("FSM rejects unknown HTTP methods") {
  FSM fsm;
  std::string input = "FOOBAR / HTTP/1.1\r\nHost: localhost\r\n\r\n";

  CHECK(!fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::NOT_IMPLEMENTED);
}
