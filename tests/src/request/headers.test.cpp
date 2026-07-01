#include "doctest.h"
#include "http/fsm.hpp"
#include <vector>

TEST_CASE("FSM allows optional headers in http/1.0") {
  FSM fsm;
  std::string input = "GET / HTTP/1.0\r\n\r\n";

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(req->headers.size() == 0);
}

TEST_CASE("FSM parses headers split across arbitrary chunks") {
  FSM fsm;

  std::vector<std::string> chunks = {"GET / HTTP/1.1\r\n",
                                     "Ho",
                                     "st:   \texa",
                                     "mple.com",
                                     "  \t\t ",
                                     "\r",
                                     "\n",
                                     "Content-Length: 0\r",
                                     "\n",
                                     "\r\n"};

  for (const auto &chunk : chunks) {
    CHECK(fsm.feedChunk(chunk.data(), chunk.length()));
  }
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->headers.size() == 2);

  const StringView *host = req->headers.get("host");
  REQUIRE(host != nullptr);
  CHECK(*host == StringView("example.com"));

  const StringView *content_len = req->headers.get("content-length");
  REQUIRE(content_len != nullptr);
  CHECK(*content_len == StringView("0"));
}

TEST_CASE("FSM handles complex header value spaces") {
  FSM fsm;
  std::string input = "GET / HTTP/1.0\r\n"
                      "User-Agent: \t Mozilla/5.0 (Windows) \t \r\n"
                      "X-Empty-Space:    \t   \r\n"
                      "\r\n";

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();

  const StringView *ua = req->headers.get("user-agent");
  REQUIRE(ua != nullptr);
  CHECK(*ua == StringView("Mozilla/5.0 (Windows)"));

  // An empty space header value should evaluate cleanly or be removed safely
  const StringView *empty_space = req->headers.get("x-empty-space");
  // Depending on your design, this should either be an empty view or invalid.
  // If your validation rejects empty values, check for bad request instead.
  if (empty_space) {
    CHECK(empty_space->empty());
  }
}
