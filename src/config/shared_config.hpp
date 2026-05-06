#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#define MAX_BODY_SIZE 1000000 // 1m

typedef std::vector<std::string> mimetype;
typedef std::map<std::string, mimetype> mimetype_map;

class SharedConfig {
private:
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
  mimetype_map types;
  // where to save uploads of locations with POST method
  std::string upload_store; // defaults to /tmp/webserve
  std::map<std::string, std::string> cgi_pass;

  friend class Config;
  friend class Server;
  friend class Location;
	friend class Webserv;

public:
  SharedConfig();

  SharedConfig &withRoot(const std::string &root);
  SharedConfig &withUploadStore(const std::string &store);
  SharedConfig &withIndex(const std::string &index);
  SharedConfig &withAutoIndex(bool on);
  SharedConfig &withErrorPage(uint16_t status, const std::string &page);
  SharedConfig &withAccessLogPath(const std::string &path);
  SharedConfig &withMimetypes(const mimetype_map &mimetypes);
  SharedConfig &withMimetype(const std::string &ext, const std::string &type);
  SharedConfig &withCgi(const std::string &lang,
                        const std::string &interpreter);
  SharedConfig &withClientMaxBodySize(size_t size);

  SharedConfig *clone() const;

  std::string toString(int indent) const;
};
