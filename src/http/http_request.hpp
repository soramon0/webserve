#pragma once

#include "request_state.hpp"
#include <map>
#include <string>

// buffering in request data while less then client max buffer size;
// move request state through parsing phases depending
// on indicators such as request-line, headers, headers-end, and body-start
// NOTE: validate it as you go or once headers are done ?
//
// NOTE: how do we move from reading client request to responding to said
// client?
//
// Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
class HttpRequest {
  std::string method;
  std::string path;
  std::string query;
  std::string protocol;
  std::map<std::string, std::string> headers;
  std::string body;

  RequestState::Type state;

  friend class Client;
  friend class Webserv;

public:
  HttpRequest();
  ~HttpRequest();

  void printRequest() const;
};
