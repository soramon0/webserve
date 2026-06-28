#include "doctest.h"
#include "http/fsm.hpp"
#include <iostream>

TEST_CASE("FSM parses complete HTTP request line") {
  FSM fsm;
  std::string input = "GET /api/products HTTP/1.1 \r\n";

  HttpRequest *req = fsm.getRequest();
  CHECK(fsm.feedChunk(input.c_str(), input.length()) == true);
  std::cout << "http status: " << req->status.asInt() << std::endl;
  REQUIRE(fsm.status.isDone());

  CHECK(req->status == HttpStatus::OK);
  CHECK(req->method.toString() == "GET");
  CHECK(std::string(req->uri.data(), req->uri.length()) == "/api/products");
  CHECK(req->version.toString() == "HTTP/1.1");
}

// TEST_CASE("FSM HTTP version") {
//   FSM fsm;
//   std::string input = "GET /api/products HTTP/1.1 \r\n";
//
//   REQUIRE(fsm.feedChunk(input.c_str(), input.length()) == true);
//   REQUIRE(fsm.status.isDone());
//
//   HttpRequest *req = fsm.getRequest();
//   CHECK(req->status == HttpStatus::OK);
//   CHECK(req->method.toString() == "GET");
//   CHECK(std::string(req->uri.data(), req->uri.length()) == "/api/products");
//   CHECK(req->version.toString() == "HTTP/1.1");
// }
