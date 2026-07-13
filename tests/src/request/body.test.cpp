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

  REQUIRE(body.init(sizeof(payload) - 1));
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

  REQUIRE(body.init(100));
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

  REQUIRE(body.init(payload_size));
  CHECK(body.isFile());
  REQUIRE(body.append(payload.data(), payload.size()));
  body.finalize();

  CHECK(body.size() == payload_size);
  CHECK(readAllBody(body) == payload);
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
