#include "doctest.h"
#include "http/fsm.hpp"
#include <vector>

TEST_CASE("FSM parses complete HTTP request line across arbitrary chunks") {
  FSM fsm;

  std::vector<std::string> chunks = {"GE", "T /api/pr", "oducts HTTP/1.1",
                                     "\r\n"};
  for (const auto &chunk : chunks) {
    CHECK(fsm.feedChunk(chunk.data(), chunk.length()));
    REQUIRE(fsm.status.isPending());
  }

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);

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
