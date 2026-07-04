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

  // An empty space header value should evaluate cleanly
  const StringView *empty_space = req->headers.get("x-empty-space");
  REQUIRE(empty_space != nullptr);
  CHECK(empty_space->empty());
}

TEST_CASE("FSM rejects malformed header keys") {
  FSM fsm;
  std::string bad_input;

  SUBCASE("Space before colon is illegal") {
    bad_input = "GET / HTTP/1.1\r\nHost : example.com\r\n\r\n";
    CHECK(!fsm.feedChunk(bad_input.data(), bad_input.length()));
  }

  SUBCASE("Space inside header key is illegal") {
    bad_input = "GET / HTTP/1.1\r\nX-My Header: value\r\n\r\n";
    CHECK(!fsm.feedChunk(bad_input.data(), bad_input.length()));
  }

  SUBCASE("Control characters in key are illegal") {
    bad_input = "GET / HTTP/1.1\r\nHost\x01: example.com\r\n\r\n";
    CHECK(!fsm.feedChunk(bad_input.data(), bad_input.length()));
  }

  REQUIRE(fsm.status.isMalformed());
  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::BAD_REQUEST);
}

TEST_CASE("FSM normalizes case-insensitive header lookup") {
  FSM fsm;
  std::string input = "GET / HTTP/1.0\r\n"
                      "coNTenT-tYPe: text/html\r\n\r\n";

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);

  CHECK(req->headers.get("content-type") != nullptr);
  CHECK(req->headers.get("coNTenT-tYPe") == nullptr);
  CHECK(req->headers.get("Content-Type") == nullptr);
  CHECK(req->headers.get("CONTENT-TYPE") == nullptr);
}

TEST_CASE("FSM handles repeated headers using multimap range") {
  FSM fsm;
  std::string input = "GET / HTTP/1.0\r\n"
                      "Accept: text/html\r\n"
                      "Accept: application/xhtml+xml\r\n"
                      "Accept:   application/xml;q=0.9   \r\n"
                      "\r\n";

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);

  // Total distinct keys might be 1, but total values stored will be 3
  CHECK(req->headers.size() == 3);

  auto range = req->headers.get_all("accept");

  // Verify we actually found the sequence
  REQUIRE(range.first != range.second);

  auto it = range.first;

  REQUIRE(it != range.second);
  CHECK(it->second == StringView("text/html"));

  ++it;
  REQUIRE(it != range.second);
  CHECK(it->second == StringView("application/xhtml+xml"));

  ++it;
  REQUIRE(it != range.second);
  CHECK(it->second == StringView("application/xml;q=0.9"));

  ++it;
  CHECK(it == range.second); // we got to end
}

TEST_CASE("FSM enforces mandatory Host header for HTTP/1.1") {
  FSM fsm;

  SUBCASE("Reject HTTP/1.1 request when Host header is missing") {
    std::string input = "GET / HTTP/1.1\r\n"
                        "Content-Length: 0\r\n\r\n";

    fsm.feedChunk(input.data(), input.length());
    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);

    CHECK(req->status == HttpStatus::BAD_REQUEST);
  }

  SUBCASE("Accept HTTP/1.1 request when Host header is present") {
    std::string input = "GET / HTTP/1.1\r\n"
                        "Host: localhost\r\n\r\n";

    CHECK(fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isDone());

    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    CHECK(req->status == HttpStatus::OK);
  }

  SUBCASE("Reject HTTP/1.1 request when Host header is empty") {
    std::string input = "GET / HTTP/1.1\r\n"
                        "Host:  \t\r\n\r\n";

    CHECK(!fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isMalformed());
    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    CHECK(req->status == HttpStatus::BAD_REQUEST);
  }
}

TEST_CASE("FSM handles arena growth, 8KB field limit, and max block counts") {
  FSM fsm;

  // HttpRequest configuration:
  // initial block = 1KB
  // max blocks = 5 (1KB + 4 * 8KB) -> Max total capacity ~33KB
  // Arena chunks are 8KB, so a single continuous field cannot exceed 8KB

  SUBCASE(
      "Arena grows correctly for a large field within the 8KB chunk limit") {
    // 6KB is well under the 8KB single-chunk limit, but forces the 1KB arena to
    // grow
    std::string large_val(6000, 'a');
    std::string input = "GET / HTTP/1.1\r\n"
                        "Host: localhost\r\n"
                        "X-Large-Header: " +
                        large_val + "\r\n\r\n";

    CHECK(fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isDone());

    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    CHECK(req->status == HttpStatus::OK);

    const StringView *lh = req->headers.get("x-large-header");
    REQUIRE(lh != nullptr);
    CHECK(lh->length() == 6000);
    CHECK(lh->data()[0] == 'a');
    CHECK(lh->data()[5999] == 'a');
  }

  SUBCASE(
      "FSM rejects request when a single header exceeds the 8KB chunk limit") {
    // 9KB exceeds the maximum capacity of a single 8KB arena growth chunk
    std::string huge_val(9000, 'b');
    std::string input = "GET / HTTP/1.1\r\n"
                        "Host: localhost\r\n"
                        "X-Huge-Header: " +
                        huge_val + "\r\n\r\n";

    CHECK(!fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isMalformed());
    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    REQUIRE(req->status == HttpStatus::REQUEST_ENTITY_TOO_LARGE);
  }

  SUBCASE("FSM rejects request when URI exceeds the 8KB chunk limit") {
    // 9KB URI exceeds the maximum capacity of a single 8KB arena growth chunk
    std::string huge_uri(9000, 'c');
    std::string input = "GET /" + huge_uri +
                        " HTTP/1.1\r\n"
                        "Host: localhost\r\n\r\n";

    CHECK(!fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isMalformed());
    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    REQUIRE(req->status == HttpStatus::REQUEST_ENTITY_TOO_LARGE);
  }

  SUBCASE("FSM enforces the max block count across multiple valid headers") {
    // We send multiple 6KB headers. Each header is valid (< 8KB),
    // but 6 headers * 6KB = 36KB total, which exceeds the max 5 blocks (~33KB
    // limit).

    std::string chunk_start = "GET / HTTP/1.1\r\nHost: localhost\r\n";
    fsm.feedChunk(chunk_start.data(), chunk_start.length());

    std::string header_val(6000, 'd');
    bool failed = false;

    for (int i = 0; i < 6; ++i) {
      std::string header_line =
          "X-Header-" + std::to_string(i) + ": " + header_val + "\r\n";
      if (!fsm.feedChunk(header_line.data(), header_line.length())) {
        failed = true;
        break; // It should fail before processing all 6 headers
      }
    }

    CHECK(failed);
    REQUIRE(fsm.status.isMalformed());
    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    REQUIRE(req->status == HttpStatus::REQUEST_ENTITY_TOO_LARGE);
  }
}
