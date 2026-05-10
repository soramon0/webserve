#include "shared_config.hpp"
#include <sstream>

SharedConfig::SharedConfig()
    : autoindex(false), client_max_body_size(MAX_BODY_SIZE),
      upload_store("/tmp/webserve") {

  this->withMimetype("html", "text/html")
      .withMimetype("htm", "text/html")
      .withMimetype("css", "text/css")
      .withMimetype("js", "application/javascript");

  this->withErrorPage(404, "./404.html")
      .withErrorPage(500, "./500.html")
      .withErrorPage(502, "./502.html")
      .withErrorPage(503, "./503.html")
      .withErrorPage(504, "./504.html");
}

SharedConfig &SharedConfig::withIndex(const std::string &index) {
  this->index.push_back(index);
  return *this;
}

SharedConfig &SharedConfig::withRoot(const std::string &root) {
  this->root = root;
  return *this;
}

SharedConfig &SharedConfig::withUploadStore(const std::string &store) {
  this->upload_store = store;
  return *this;
}

SharedConfig &SharedConfig::withAutoIndex(bool on) {
  this->autoindex = on;
  return *this;
}

SharedConfig &SharedConfig::withErrorPage(uint16_t status,
                                          const std::string &page) {
  this->error_page[status] = page;
  return *this;
}

SharedConfig &SharedConfig::withAccessLogPath(const std::string &path) {
  this->access_log_path = path;
  return *this;
}

SharedConfig &SharedConfig::withCgi(const std::string &lang,
                                    const std::string &interpreter) {

  this->cgi_pass[lang] = interpreter;
  return *this;
}

SharedConfig &SharedConfig::withMimetypes(const mimetype_map &mimetypes) {
  this->types = mimetypes;
  return *this;
}

SharedConfig &SharedConfig::withClientMaxBodySize(size_t size) {
  this->client_max_body_size = size;
  return *this;
}

SharedConfig &SharedConfig::withMimetype(const std::string &ext,
                                         const std::string &type) {
  this->types[type].push_back(ext);
  return *this;
}

SharedConfig *SharedConfig::clone() const {
  SharedConfig *clone = new SharedConfig(*this);
  return clone;
}

std::string SharedConfig::toString(int indent) const {
  std::string tab = std::string(indent, '\t');
  std::ostringstream oss;

  oss << tab << "root " << root << ";\n";
  oss << tab << "autoindex " << (this->autoindex ? "on" : "off") << ";\n";

  if (this->index.size() > 0) {
    oss << tab << "index ";
    for (size_t i = 0; i < this->index.size(); i++) {
      oss << this->index[i];
      if (i + 1 < this->index.size()) {
        oss << " ";
      } else {
        oss << ";\n";
      }
    }
  }

  oss << tab << "client_max_body_size " << this->client_max_body_size << ";\n";
  oss << tab << "upload_store " << this->upload_store << ";\n";

  if (this->access_log_path.empty()) {
    oss << tab << "access_log off;\n";
  } else {
    oss << tab << "access_log " << this->access_log_path << ";\n";
  }

  for (std::map<int, std::string>::const_iterator it = this->error_page.begin();
       it != this->error_page.end(); ++it) {
    oss << tab << "error_page " << it->first << " " << it->second << ";\n";
  }

  if (this->types.size() > 0) {
    oss << tab << "mime_types {\n";
    for (mimetype_map::const_iterator it = this->types.begin();
         it != this->types.end(); ++it) {
      oss << tab << "\t" << it->first << "\t";
      for (size_t i = 0; i < it->second.size(); i++) {
        oss << " " << it->second[i];
      }
      oss << ";\n";
    }
    oss << tab << "}\n";
  } else {
    oss << tab << "mime_types {}\n";
  }

  if (this->cgi_pass.size() > 0) {
    oss << tab << "cgi_pass {\n";
    for (std::map<std::string, std::string>::const_iterator it =
             this->cgi_pass.begin();
         it != this->cgi_pass.end(); ++it) {
      oss << tab << "\t" << it->first << "\t" << it->second << ";\n";
    }
    oss << tab << "}\n";
  } else {
    oss << tab << "cgi_pass {}";
  }

  return oss.str();
}
