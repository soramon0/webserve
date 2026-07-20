#include "common.h"
#include "config/server.hpp"
#include "doctest.h"
#include "http/fsm.hpp"
#include "http/request_body.hpp"
#include <cstring>
#include <sstream>
#include <string>

static std::string makePostRequest(const std::string &path,
                                   const std::string &body) {
  std::ostringstream os;
  os << "POST " << path << " HTTP/1.1\r\n";
  os << "Host: localhost\r\n";
  os << "Content-Length: " << body.size() << "\r\n";
  os << "\r\n";
  os << body;
  return os.str();
}

static std::string toHex(size_t n) {
  std::ostringstream os;
  os << std::hex << n;
  return os.str();
}

static std::string makeChunkedPost(const std::string &path,
                                   const std::string &body,
                                   size_t chunk_len = 0) {
  std::ostringstream os;
  os << "POST " << path << " HTTP/1.1\r\n";
  os << "Host: localhost\r\n";
  os << "Transfer-Encoding: chunked\r\n";
  os << "\r\n";

  if (body.empty()) {
    os << "0\r\n\r\n";
    return os.str();
  }

  if (chunk_len == 0) {
    chunk_len = body.size();
  }

  for (size_t offset = 0; offset < body.size(); offset += chunk_len) {
    size_t len = chunk_len;
    if (offset + len > body.size()) {
      len = body.size() - offset;
    }
    os << toHex(len) << "\r\n";
    os.write(body.data() + offset, static_cast<std::streamsize>(len));
    os << "\r\n";
  }
  os << "0\r\n\r\n";
  return os.str();
}

static Location makeUploadLocation() {
  Location loc;
  loc.withPath("/upload");
  loc.withMethod("POST");
  return loc;
}

static void setupUploadServer(Server &server, size_t max_body) {
  server.withSharedConfig(SharedConfig().withClientMaxBodySize(max_body));
  server.withLocation("/upload", makeUploadLocation());
}

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

static std::string readAllBody(RequestBody &body) {
  std::string out;
  body.resetReader();

  for (;;) {
    RequestBody::ReadResult res = body.read();
    REQUIRE(res.status != RequestBody::READ_ERROR);

    if (res.block) {
      out.append(reinterpret_cast<const char *>(res.block->getBuffer()),
                 res.block->consumed());
    }

    if (res.status == RequestBody::READ_DONE) {
      break;
    }
  }

  return out;
}

TEST_CASE("RequestBody stores small payloads in memory") {
  RequestBody body;
  const char payload[] = "{\"name\": \"karim\"}";

  REQUIRE(body.init(sizeof(payload) - 1, false));
  CHECK(!body.isFile());
  REQUIRE(body.append(payload, sizeof(payload) - 1));
  body.finalize();

  CHECK(body.size() == sizeof(payload) - 1);
  CHECK(std::memcmp(body.getBuffer(), payload, body.size()) == 0);

  std::string read_back = readAllBody(body);
  CHECK(read_back == payload);
}

TEST_CASE(
    "RequestBody spills to a temp file when memory capacity is exceeded") {
  RequestBody body;
  const size_t first_chunk = KIB(16);
  const size_t extra = 128;
  std::string payload(first_chunk + extra, 'a');

  REQUIRE(body.init(100, false));
  REQUIRE(body.append(payload.data(), first_chunk));
  CHECK(!body.isFile());

  REQUIRE(body.append(payload.data() + first_chunk, extra));
  CHECK(body.isFile());
  body.finalize();

  CHECK(body.size() == payload.size());
  CHECK(readAllBody(body) == payload);
}

TEST_CASE("RequestBody uses file mode when declared size exceeds memory cap") {
  RequestBody body;
  const size_t payload_size = KIB(16) + 1;
  std::string payload(payload_size, 'b');

  REQUIRE(body.init(payload_size, false));
  CHECK(body.isFile());
  REQUIRE(body.append(payload.data(), payload.size()));
  body.finalize();

  CHECK(body.size() == payload_size);
  CHECK(readAllBody(body) == payload);
}

TEST_CASE("RequestBody uses file mode when useDisk is set") {
  RequestBody body;
  const char payload[] = "cgi-body";

  REQUIRE(body.init(sizeof(payload) - 1, true));
  CHECK(body.isFile());
  REQUIRE(body.append(payload, sizeof(payload) - 1));
  body.finalize();

  CHECK(body.size() == sizeof(payload) - 1);
  CHECK(readAllBody(body) == payload);
}

TEST_CASE("RequestBody::fitsContentLength rejects overflow past Content-Length") {
  RequestBody body;
  REQUIRE(body.init(8, false));
  REQUIRE(body.append("12345", 5));

  CHECK(body.fitsContentLength(3, 8));
  CHECK(!body.fitsContentLength(4, 8));
  CHECK(!body.fitsContentLength(1, 4));
}

TEST_CASE("FSM accepts POST body delivered in the same chunk as headers") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string payload = "{\"name\": \"karim\"}";
  const std::string input = makePostRequest("/upload", payload);

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(req->body.size() == payload.size());
  CHECK(!req->body.isFile());
  CHECK(readAllBody(req->body) == payload);
}

TEST_CASE("FSM accepts POST body split across multiple chunks") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string payload = "{\"name\": \"karim\"}";
  const std::string headers =
      "POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: " +
      std::to_string(payload.size()) + "\r\n\r\n";

  CHECK(fsm.feedChunk(headers.data(), headers.length()));
  REQUIRE(fsm.status.isPending());

  CHECK(fsm.feedChunk(payload.data(), payload.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(readAllBody(req->body) == payload);
}

TEST_CASE("FSM accepts POST body using production-like chunk sizes") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string payload(64, 'z');
  const std::string input = makePostRequest("/upload", payload);

  feedInChunks(fsm, input, KIB(1) / 2);
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(readAllBody(req->body) == payload);
}

TEST_CASE("FSM rejects POST without Content-Length or Transfer-Encoding") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string input = "POST /upload HTTP/1.1\r\nHost: localhost\r\n\r\n";

  CHECK(!fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::BAD_REQUEST);
}

TEST_CASE("FSM rejects POST when body exceeds client_max_body_size") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 10);
  fsm.setServer(&server);

  const std::string payload(11, 'x');
  const std::string input = makePostRequest("/upload", payload);

  CHECK(!fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::REQUEST_ENTITY_TOO_LARGE);
  CHECK(req->error == "request greater than client_max_body_size");
}

TEST_CASE("FSM rejects POST when received body exceeds Content-Length") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string headers =
      "POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\n";
  const std::string body = "123456";

  CHECK(fsm.feedChunk(headers.data(), headers.length()));
  REQUIRE(fsm.status.isPending());

  CHECK(!fsm.feedChunk(body.data(), body.length()));
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::REQUEST_ENTITY_TOO_LARGE);
  CHECK(req->error == "request greater than content-length");
}

TEST_CASE("FSM rejects POST when location does not allow the method") {
  FSM fsm;
  Server server;

  Location loc;
  loc.withPath("/upload");
  loc.withMethod("GET");
  server.withSharedConfig(SharedConfig().withClientMaxBodySize(100));
  server.withLocation("/upload", loc);
  fsm.setServer(&server);

  const std::string payload = "hello";
  const std::string input = makePostRequest("/upload", payload);

  CHECK(!fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::METHOD_NOT_ALLOWED);
}

TEST_CASE("FSM accepts chunked POST body") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string payload = "Hello, this is a chunked request body text.";
  const std::string input = makeChunkedPost("/upload", payload, 10);

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(req->body.size() == payload.size());
  CHECK(readAllBody(req->body) == payload);
}

TEST_CASE("FSM accepts empty chunked POST body") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string input = makeChunkedPost("/upload", "");

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(req->body.size() == 0);
}

TEST_CASE("FSM accepts chunked POST with chunk extensions") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string input = "POST /upload HTTP/1.1\r\n"
                            "Host: localhost\r\n"
                            "Transfer-Encoding: chunked\r\n"
                            "\r\n"
                            "5;foo=bar\r\n"
                            "hello\r\n"
                            "0\r\n"
                            "\r\n";

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(readAllBody(req->body) == "hello");
}

TEST_CASE("FSM accepts chunked POST split across feedChunk calls") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string payload = "abcdef";
  const std::string input = makeChunkedPost("/upload", payload, 3);

  feedInChunks(fsm, input, 1);
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(readAllBody(req->body) == payload);
}

TEST_CASE("FSM rejects chunked POST when body exceeds client_max_body_size") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 10);
  fsm.setServer(&server);

  const std::string payload(11, 'x');
  const std::string input = makeChunkedPost("/upload", payload);

  CHECK(!fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::REQUEST_ENTITY_TOO_LARGE);
  CHECK(req->error == "request chunk greater than max body size");
}

TEST_CASE("FSM rejects chunked POST with bad CRLF after chunk data") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string input = "POST /upload HTTP/1.1\r\n"
                            "Host: localhost\r\n"
                            "Transfer-Encoding: chunked\r\n"
                            "\r\n"
                            "5\r\n"
                            "helloX"
                            "0\r\n"
                            "\r\n";

  CHECK(!fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::BAD_REQUEST);
}

TEST_CASE("FSM rejects chunked POST with overflowing chunk size") {
  FSM fsm;
  Server server;
  setupUploadServer(server, SIZE_MAX);
  fsm.setServer(&server);

  std::string huge_size(17, 'f'); // 17 hex digits overflows 64-bit size_t
  const std::string input = "POST /upload HTTP/1.1\r\n"
                            "Host: localhost\r\n"
                            "Transfer-Encoding: chunked\r\n"
                            "\r\n" +
                            huge_size + "\r\n";

  CHECK(!fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::BAD_REQUEST);
}

TEST_CASE("FSM rejects chunked POST with empty chunk size line") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  // Empty size before CRLF must not be treated as the final 0-chunk
  const std::string input = "POST /upload HTTP/1.1\r\n"
                            "Host: localhost\r\n"
                            "Transfer-Encoding: chunked\r\n"
                            "\r\n"
                            "5\r\n"
                            "hello\r\n"
                            "\r\n\r\n";

  CHECK(!fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::BAD_REQUEST);
  CHECK(req->error == "chunk size required");
}

TEST_CASE("FSM rejects chunked POST trailer fields") {
  FSM fsm;
  Server server;
  setupUploadServer(server, 100);
  fsm.setServer(&server);

  const std::string input = "POST /upload HTTP/1.1\r\n"
                            "Host: localhost\r\n"
                            "Transfer-Encoding: chunked\r\n"
                            "\r\n"
                            "5\r\n"
                            "hello\r\n"
                            "0\r\n"
                            "X-Injected: evil\r\n"
                            "\r\n";

  CHECK(!fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isMalformed());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::BAD_REQUEST);
  CHECK(req->error == "trailers are not supported");
  CHECK(req->headers.get("x-injected") == nullptr);
}

TEST_CASE("FSM accepts multi-chunk body with useDisk") {
  FSM fsm;
  Server server;
  server.withSharedConfig(
      SharedConfig().withClientMaxBodySize(100).withCgi("py", "/bin/true"));
  server.withLocation("/upload", makeUploadLocation());
  fsm.setServer(&server);

  const std::string payload = "disk-chunked-body";
  const std::string input = makeChunkedPost("/upload", payload, 4);

  CHECK(fsm.feedChunk(input.data(), input.length()));
  REQUIRE(fsm.status.isDone());

  HttpRequest *req = fsm.getRequest();
  REQUIRE(req != nullptr);
  CHECK(req->status == HttpStatus::OK);
  CHECK(req->body.isFile());
  CHECK(readAllBody(req->body) == payload);
}
