#include "common.h"
#include "doctest.h"
#include "http/fsm.hpp"
#include <string>

static void feedInChunks(FSM &fsm, const std::string &input,
                         size_t chunk_size) {
  for (size_t offset = 0; offset < input.length(); offset += chunk_size) {
    size_t len = chunk_size;
    if (offset + len > input.length()) {
      len = input.length() - offset;
    }
    CHECK(fsm.feedChunk(input.data() + offset, len));
  }
}

TEST_CASE("FSM parses large header split into single-byte chunks") {
  FSM fsm;

  std::string prefix = "GET / HTTP/1.1\r\nHost: localhost\r\nX-Large: ";
  std::string suffix = "\r\n\r\n";
  std::string value(6000, 'z');
  std::string input = prefix + value + suffix;

  for (size_t i = 0; i < input.length(); ++i) {
    CHECK(fsm.feedChunk(&input[i], 1));
  }
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);

  const StringView *lh = req->headers.get("x-large");
  REQUIRE(lh != nullptr);
  CHECK(lh->length() == 6000);
  CHECK(lh->data()[0] == 'z');
  CHECK(lh->data()[5999] == 'z');
}

TEST_CASE("FSM preserves earlier fields after arena grow") {
  FSM fsm;

  std::string large_val(6000, 'q');
  std::string input = "GET /api HTTP/1.1\r\n"
                      "Host: example.com\r\n"
                      "X-Large: " +
                      large_val + "\r\n\r\n";

  for (size_t i = 0; i < input.length(); ++i) {
    CHECK(fsm.feedChunk(&input[i], 1));
  }
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);

  CHECK(req->method.toString() == "GET");
  CHECK(std::string(req->uri.data(), req->uri.length()) == "/api");

  const StringView *host = req->headers.get("host");
  REQUIRE(host != nullptr);
  CHECK(*host == StringView("example.com"));
}

TEST_CASE("FSM parses large request using production-like 512-byte chunks") {
  FSM fsm;

  std::string large_val(6000, 'p');
  std::string input = "GET / HTTP/1.1\r\n"
                      "Host: localhost\r\n"
                      "X-Large: " +
                      large_val + "\r\n\r\n";

  feedInChunks(fsm, input, KIB(1) / 2);
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);

  const StringView *lh = req->headers.get("x-large");
  REQUIRE(lh != nullptr);
  CHECK(lh->length() == 6000);
}

TEST_CASE("FSM restart resets parser state") {
  FSM fsm;

  CHECK(fsm.feedChunk("GET /part", 9));
  REQUIRE(fsm.status.isPending());

  fsm.restart();

  std::string input = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(std::string(req->uri.data(), req->uri.length()) == "/");
}

TEST_CASE("FSM rejects request when total header storage exceeds limit") {
  FSM fsm;

  std::string chunk_start = "GET / HTTP/1.1\r\nHost: localhost\r\n";
  CHECK(fsm.feedChunk(chunk_start.data(), chunk_start.length()));

  std::string header_val(6000, 'd');
  bool failed = false;

  for (int i = 0; i < 6; ++i) {
    std::string header_line =
        "X-Header-" + std::to_string(i) + ": " + header_val + "\r\n";
    if (!fsm.feedChunk(header_line.data(), header_line.length())) {
      failed = true;
      break;
    }
  }

  CHECK(failed);
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::REQUEST_ENTITY_TOO_LARGE);
}
