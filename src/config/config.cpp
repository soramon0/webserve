#include "config.hpp"
#include <sstream>

Config::Config() { this->shared_config = new SharedConfig(); };

Config::~Config() { delete shared_config; }

Config::Config(const Config &other) {
  this->servers = other.servers;
  if (other.shared_config) {
    this->shared_config = other.shared_config->clone();
  } else {
    this->shared_config = new SharedConfig();
  }
}

Config &Config::operator=(const Config &other) {
  if (this == &other) {
    return *this;
  }

  if (this->shared_config) {
    delete this->shared_config;
    this->shared_config = NULL;
  }

  this->servers = other.servers;

  if (other.shared_config) {
    this->shared_config = other.shared_config->clone();
  }
  return *this;
}

Config &Config::withServer(const Server &server) {
  Server *srv = new Server(server);

  if (this->shared_config) {
    srv->withSharedConfig(*this->shared_config);
  }

  this->servers.push_back(srv);
  return *this;
}

Config &Config::withSharedConfig(const SharedConfig &cfg) {
  if (this->shared_config) {
    delete this->shared_config;
  }

  this->shared_config = cfg.clone();
  return *this;
}

bool Config::hasServer(const Server &srv) const {
  if (servers.empty())
    return false;

  std::vector<Server *>::const_iterator it;
  for (it = servers.begin(); it != servers.end(); it++) {
    if ((*it)->interface == srv.interface && (*it)->port == srv.port) {
      return true;
    }
  }
  return false;
}

std::string Config::toString() const {
  std::ostringstream oss;

  oss << "http {\n";

  if (this->shared_config) {
    oss << this->shared_config->toString(1);
  }

  if (this->servers.size() > 0) {
    for (std::vector<Server *>::const_iterator it = this->servers.begin();
         it != this->servers.end(); ++it) {
      oss << (*it)->toString(1);
    }
  }

  oss << "}";
  return oss.str();
}

void Config::resolveSharedConfigs() {
  SharedConfig defaultCfg =
      SharedConfig()
          .withRoot("nginx/www")
          .withClientMaxBodySize(static_cast<size_t>(MAX_CLIENT_BODY_SIZE))
          .withMimetype("html", "text/html")
          .withMimetype("htm", "text/html")
          .withMimetype("css", "text/css")
          .withMimetype("js", "application/javascript")
          .withMimetype("jpg", "image/jpeg")
          .withMimetype("jpeg", "image/jpeg")
          .withMimetype("png", "image/png")
          .withMimetype("svg", "image/svg+xml");

  SharedConfig *cfg = SharedConfig::merge(defaultCfg, *this->shared_config);

  delete this->shared_config;
  this->shared_config = cfg;

  for (size_t i = 0; i < servers.size(); ++i) {
    Server *srv = servers[i];

    SharedConfig *srvCfg =
        SharedConfig::merge(*this->shared_config, *srv->shared_config);

    delete srv->shared_config;
    srv->shared_config = srvCfg;

    for (std::map<std::string, Location>::iterator it = srv->locations.begin();
         it != srv->locations.end(); ++it) {
      Location &loc = it->second;
      SharedConfig *locCfg =
          SharedConfig::merge(*srv->shared_config, *loc.shared_config);

      delete loc.shared_config;
      loc.shared_config = locCfg;
    }
  }
}
