#pragma once
#include "server.hpp"
#include <string>
#include <vector>

#define MAX_CLIENT_BODY_SIZE 1000000 // 1m

struct Config {
  std::vector<Server *> servers;
  SharedConfig *shared_config;

  ~Config();
  Config();

  Config &withServer(const Server &server);
  Config &withSharedConfig(const SharedConfig &cfg);

  std::string toString() const;

  bool hasServer(const Server &srv) const;

  void resolveSharedConfigs();

  Config(const Config &other);
  Config &operator=(const Config &other);
};
