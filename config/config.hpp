#include <map>
#include <string>
#include <vector>

class ReturnDir {
  int code;        // optional, default to temporary redirect 302
  std::string url; // redirect url
};

class SharedConfig {
  std::string root;
  std::vector<std::string> index;
  // directory listing if path matches and has a trailing /
  // autoindex off by default. index has precedence over autoindex.
  bool autoindex;
  // collect `error_page 500 502 503 504 ./50x.html` into a map
  // {500: "./50x.html", 502: "./50x.html", 503: "./50x.html", 504:"./50x.html"}
  std::map<int, std::string> error_page;
  size_t client_max_body_size;
  // empty string is off but if we have a string then it's the path where to
  // save the access log
  std::string access_log_path;
  std::map<std::string, std::vector<std::string>> types;
  // where to save uploads of locations with POST method
  std::string upload_store; // defaults to /tmp/webserve
  std::map<std::string, std::string> cgi_pass;
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
  std::vector<Server> servers;
  SharedConfig *shared_config;
  //   Logger *l
};
