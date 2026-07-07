#include "location.hpp"
#include "lib/utils.hpp"
#include "redirect.hpp"
#include <sstream>

Location::Location() : path(), return_rule(NULL) {
  shared_config = new SharedConfig();
  methods.push_back("get");
};

Location::Location(const std::string &path) : path(path), return_rule(NULL) {
  shared_config = new SharedConfig();
  methods.push_back("get");
};

Location::~Location() {
  if (this->return_rule) {
    delete this->return_rule;
  }

  if (this->shared_config) {
    delete this->shared_config;
  }
};

Location::Location(const Location &other)
    : path(other.path), return_rule(NULL), methods(other.methods) {
  if (other.return_rule) {
    this->return_rule = new ReturnDir(*other.return_rule);
  }

  if (other.shared_config) {
    this->shared_config = other.shared_config->clone();
  } else {
    this->shared_config = new SharedConfig();
  }
}

Location &Location::operator=(const Location &other) {
  if (this == &other) {
    return *this;
  }

  this->path = other.path;
  this->methods = other.methods;

  if (this->return_rule) {
    delete this->return_rule;
    this->return_rule = NULL;
  }
  if (this->shared_config) {
    delete this->shared_config;
    this->shared_config = NULL;
  }

  if (other.return_rule) {
    this->return_rule = new ReturnDir(*other.return_rule);
  }
  if (other.shared_config) {
    this->shared_config = other.shared_config->clone();
  }

  return *this;
}

Location &Location::withPath(const std::string &path) {
  this->path = path;
  return *this;
}

Location &Location::withRedirect(uint16_t code, const std::string &url) {
  if (this->return_rule) {
    delete this->return_rule;
  }

  this->return_rule = new ReturnDir(code, url);
  return *this;
}

Location &Location::withRedirect(const std::string &url) {
  if (this->return_rule) {
    delete this->return_rule;
  }

  this->return_rule = new ReturnDir(url);
  return *this;
}

Location &Location::withSharedConfig(const SharedConfig &cfg) {
  if (this->shared_config) {
    delete this->shared_config;
  }

  this->shared_config = cfg.clone();
  return *this;
}

std::string Location::toString(int indent) const {
  std::ostringstream oss;
  std::string tab = std::string(indent, '\t');

  oss << tab << "location " << this->path << " {\n";
  if (this->return_rule) {
    oss << tab << "\treturn " << this->return_rule->code << " "
        << this->return_rule->url << ";\n";
  }

  oss << tab << "\tmethods ";
  if (!this->methods.empty()) {
    for (size_t i = 0; i < this->methods.size(); i++) {
      oss << strToUpper(this->methods[i]);
      if (i + 1 < this->methods.size()) {
        oss << " ";
      } else {
        oss << ";\n";
      }
    }
  }

  if (this->shared_config) {
    oss << this->shared_config->toString(indent + 1);
  }
  oss << tab << "}\n";
  return oss.str();
}
