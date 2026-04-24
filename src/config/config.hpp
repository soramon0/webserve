#include "shared_config.hpp"
#include <map>
#include <string>
#include <vector>

class ReturnDir {
  int code;        // optional, default to temporary redirect 302
  std::string url; // redirect url
};

class Location {
  std::string path;
  ReturnDir *return_rule;

  // copy from parent (server) and update if needed to match location
  SharedConfig *shared_config;
};

class Server {
  // std::string server_name; virtual hosts
  int port;
  std::string interface;
  std::map<std::string, Location> locations;
  ReturnDir *return_rule;

  // copy from parent (http) and update if needed to match server
  SharedConfig *shared_config;
};

//  global config
class Config {
public:
  std::vector<Server> servers;
  SharedConfig *shared_config;

  ~Config();
  Config();

  void assignSharedConfig(SharedConfig *shared_config);

  std::string toString() const;

private:
  // Only won config should be created so disable copying
  Config(const Config &other);
  Config &operator=(const Config &other);
};
