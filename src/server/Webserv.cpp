#include "Webserv.hpp"
#include "logger/log.hpp"
#include "utils.hpp"
#include <cstring>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>

static int running = true;

Webserv::Webserv(Config _conf) : config(_conf) {}

Webserv::~Webserv() {
  close(epoll_fd);
  // close all sockets before

  while (!clients.empty()) {
    removeClient(clients.begin()->first);
  }

  std::map<SOCKET, Server *>::iterator it_srv = servers.begin();
  while (it_srv != servers.end()) {
    close(it_srv->first);
    ++it_srv;
  }
}

void Webserv::start() {
  int srvlen = config.servers.size();

  epoll_fd = epoll_instance();
  for (int i = 0; i < srvlen; i++) {
    SOCKET listen_sock = createSocket(i);
    if (set_nonblocking(listen_sock) == -1) {
      Logger::error("Can't set non-blocking");
      continue;
    }
    if (add_to_epoll(epoll_fd, listen_sock, EPOLLIN) == -1)
      continue;
    servers[listen_sock] = config.servers[i];
  }

  if (servers.size() == 0) {
    Logger::error("could not register servers.");
    return;
  }

  eventLoop();
}

void sigHandler(int sig) {
  (void)sig;
  running = false;
}

void Webserv::eventLoop() {
  struct epoll_event events[MAX_EVENTS];

  signal(SIGINT, sigHandler);
  signal(SIGTERM, sigHandler);

  while (running) {
    int n_ev = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    if (n_ev <= 0) // possible error : EINTR
      continue;

    int ev;
    SOCKET fd;
    for (int i = 0; i < n_ev; i++) {
      ev = events[i].events;
      fd = static_cast<SOCKET>(events[i].data.fd);

      if (ev & (EPOLLERR | EPOLLHUP)) {
        removeClient(fd);
        continue;
      }

      if (ev & EPOLLIN) {
        if (servers.count(fd))
          handleNewConnection(fd);
        else
          handleClientData(fd);
      }

      if (ev & EPOLLOUT) {
        handleHttpResponse(fd);
      }
    }
  }
}

SOCKET Webserv::createSocket(int id) {
  struct addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  std::ostringstream os;
  os << config.servers[id]->port;
  std::string port = os.str();
  std::string host = config.servers[id]->interface;

  struct addrinfo *addr;
  if (getaddrinfo(host.c_str(), port.c_str(), &hints, &addr)) {
    Logger::fatal("getaddrinfo failed");
  }

  int socket_listen =
      socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
  if (!ISVALIDSOCKET(socket_listen))
    Logger::fatal("socket failed");

  int opt = 1;
  if (setsockopt(socket_listen, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt)))
    Logger::error("setsockopt failed");

  if (bind(socket_listen, addr->ai_addr, addr->ai_addrlen))
    Logger::fatal("bind failed");
  freeaddrinfo(addr);

  Logger::info("Listening on http://%s:%s ...", host.c_str(), port.c_str());
  if (listen(socket_listen, SOMAXCONN))
    Logger::fatal("listen failed");

  return socket_listen;
}

void Webserv::handleNewConnection(SOCKET srv) {
  int max_accepts = 32;

  while (max_accepts-- > 0) {
    Client *c = new Client();
    c->socket = accept(srv, &c->addr, &c->addrlen);
    c->received = 0;

    if (c->socket == -1) {
      delete c;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;

      Logger::error("accept failed");
      break;
    }
    if (set_nonblocking(c->socket) == -1) {
      Logger::error("fcntl failed");
      delete c;
      close(c->socket);
      continue;
    }
    if (add_to_epoll(epoll_fd, c->socket, EPOLLIN) == -1) {
      delete c;
      continue;
    }
    c->srv = servers[srv];
    c->machine.setServer(c->srv);
    clients[c->socket] = c;
    Logger::info("client Connected...");
  }
}

void Webserv::handleClientData(SOCKET c) {
  Client *cl = clients[c];

  char buf[KIB(1) / 2];
  ssize_t bytes = recv(cl->socket, buf, sizeof(buf), 0);
  if (bytes <= 0) {
    removeClient(c);
    return;
  }

  HttpRequest *req = cl->machine.getRequest();
  if (cl->machine.status.isPending() && !cl->machine.feedChunk(buf, bytes)) {
    Logger::debug("request status: %d", req->status.asInt());
    Logger::debug("request error: %.*s", (int)req->error.length(),
                  req->error.data());
  }

  // notify client to send a response
  if (!cl->machine.status.isPending()) {
    req->printRequest();
    modify_epoll(epoll_fd, c, EPOLLOUT);
  }
}

void Webserv::removeClient(SOCKET c) {
  Logger::debug("dropping client(%d)", c);

  std::map<SOCKET, Client *>::iterator it = clients.find(c);
  if (it == clients.end()) {
    return;
  }

  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, c, NULL);
  Client *cl = it->second;
  clients.erase(c);
  close(c);
  delete cl;
}

void Webserv::handleHttpResponse(SOCKET c) {
  FSMStatus status = clients[c]->machine.status;

  if (status.isPending())
    return;

  HttpRequest *req = clients[c]->machine.getRequest();
  if (status.isMalformed()) {
    // request had an error
    std::ostringstream stream;

    stream << req->version.toString();
    stream << " ";
    stream << req->status.asInt();
    stream << " ";
    stream << HttpStatus::reasonPhrase(req->status.value());
    stream << "\r\n";
    stream << "Content-Type: text/plain\r\n";
    stream << "Content-Length: ";
    stream << req->error.length();
    stream << "\r\n\r\n";
    stream << req->error;
    std::string response = stream.str();
    int n = send(c, response.c_str(), response.size(), 0);
    (void)n;
  } else {
    std::string response = HELLO_WORLD_RESPONSE;

    int n = send(c, response.c_str(), response.size(), 0);
    (void)n;
  }

  removeClient(c);
}
