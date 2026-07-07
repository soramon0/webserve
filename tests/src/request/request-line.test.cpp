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

TEST_CASE("FSM correctly extracts path from Absolute URI format") {
  FSM fsm;
  std::string input = "GET https://example.com/api/products HTTP/1.1\r\n";

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isPending());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(req->method.toString() == "GET");

  CHECK(std::string(req->uri.data(), req->uri.length()) == "/api/products");
  CHECK(req->version.toString() == "HTTP/1.1");
}

TEST_CASE("FSM parses absolute URI split across chunks") {
  FSM fsm;

  std::vector<std::string> chunks = {"GET htt", "ps://examp",
                                     "le.com/api/products HT", "TP/1.1\r\n"};
  for (const auto &chunk : chunks) {
    CHECK(fsm.feedChunk(chunk.data(), chunk.length()));
    REQUIRE(fsm.status.isPending());
  }

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(std::string(req->uri.data(), req->uri.length()) == "/api/products");
}

TEST_CASE("FSM handles absolute URI with missing trailing path") {
  FSM fsm;
  std::string input = "GET http://example.com HTTP/1.1\r\n";

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isPending());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(std::string(req->uri.data(), req->uri.length()) == "/");
}

TEST_CASE("FSM handles incomplete CRLF") {
  FSM fsm;
  std::string input = "GET / HTTP/1.0\r";

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isPending());
  CHECK(fsm.feedChunk("\n", 1));
  REQUIRE(fsm.status.isPending());
  CHECK(fsm.feedChunk("\r\n", 2));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(req->version == HttpVersion::V1_0);
  CHECK(std::string(req->uri.data(), req->uri.length()) == "/");
}

TEST_CASE("FSM rejects missing uri") {
  FSM fsm;
  std::string bad_input = "GET\r\n";
  size_t error_index = bad_input.find('\r');

  for (size_t i = 0; i < 3; ++i) {
    CHECK(fsm.feedChunk(&bad_input[i], 1));
    REQUIRE(fsm.status.isPending());
  }

  CHECK(!fsm.feedChunk(&bad_input[error_index], 1));
  CHECK(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::BAD_REQUEST);
}

TEST_CASE("FSM rejects missing HTTP version") {
  FSM fsm;
  std::string bad_input = "GET /api/products\r\n";
  size_t error_index = bad_input.find('\r');

  for (size_t i = 0; i < error_index; ++i) {
    CHECK(fsm.feedChunk(&bad_input[i], 1));
    REQUIRE(fsm.status.isPending());
  }

  CHECK(!fsm.feedChunk(&bad_input[error_index], 1));
  CHECK(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::BAD_REQUEST);
}

TEST_CASE("FSM correctly rejects various malformed request lines") {
  FSM fsm;
  std::string bad_input;

  SUBCASE("Missing space between method and URI") {
    bad_input = "GET/api/productsHTTP/1.1\r\n";
    CHECK(!fsm.feedChunk(bad_input.data(), bad_input.length()));
  }

  SUBCASE("Tabs instead of spaces") {
    bad_input = "GET\t/api/products\tHTTP/1.1\r\n";
    CHECK(!fsm.feedChunk(bad_input.data(), bad_input.length()));
  }

  SUBCASE("Multiple spaces between method and URI") {
    bad_input = "GET  /api/products HTTP/1.1\r\n";
    CHECK(!fsm.feedChunk(bad_input.data(), bad_input.length()));
  }

  SUBCASE("Malformed HTTP version format") {
    bad_input = "GET /api/products http/1.1\r\n";
    CHECK(!fsm.feedChunk(bad_input.data(), bad_input.length()));
  }

  SUBCASE("Empty request-line") {
    bad_input = "";
    CHECK(!fsm.feedChunk(bad_input.data(), bad_input.length()));
  }

  SUBCASE("Unsupported URI Scheme") {
    bad_input = "GET ftp://example.com/api/products HTTP/1.1\r\n";
    CHECK(!fsm.feedChunk(bad_input.data(), bad_input.length()));
  }

  SUBCASE("Missing domain name in absolute URI") {
    bad_input = "GET https:// HTTP/1.1\r\n";
    CHECK(!fsm.feedChunk(bad_input.data(), bad_input.length()));
  }

  REQUIRE(fsm.status.isMalformed());
  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::BAD_REQUEST);
}

TEST_CASE("FSM keeps request line within the initial 1KB arena block") {
  FSM fsm;

  SUBCASE("FSM accepts request line that fills the 1KB block") {
    // GET(3) + uri(1013) + HTTP/1.1(8) = 1024 bytes in the first block
    std::string uri_body(1012, 'u');
    std::string input =
        "GET /" + uri_body + " HTTP/1.1\r\nHost: localhost\r\n\r\n";

    CHECK(fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isDone());

    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    CHECK(req->status == HttpStatus::OK);
    CHECK(req->uri.length() == 1013);
  }

  SUBCASE("FSM rejects request line that exceeds the 1KB block") {
    std::string uri_body(1013, 'v');
    std::string input = "GET /" + uri_body + " HTTP/1.1\r\n";

    CHECK(!fsm.feedChunk(input.data(), input.length()));
    REQUIRE(fsm.status.isMalformed());

    HttpRequest *req = fsm.getRequest();
    REQUIRE(req != nullptr);
    CHECK(req->status == HttpStatus::URI_TOO_LONG);
  }
}
