#pragma once
#include "server.hpp"
#include <string>
#include <vector>

class Config {
public:
  std::vector<Server> servers;
  SharedConfig *shared_config;

  ~Config();
  Config();

  Config &withServer(const Server &server);
  Config &withSharedConfig(const SharedConfig &cfg);

  std::string toString() const;

private:
  // Only won config should be created so disable copying
  // Config(const Config &other);
  // Config &operator=(const Config &other);
};
