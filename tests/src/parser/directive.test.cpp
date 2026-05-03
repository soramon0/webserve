#include "../utils.hpp"
#include "config/config.hpp"
#include "parser/parser.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <stdexcept>

namespace {

struct UnlinkAtScopeEnd {
  std::string path;
  explicit UnlinkAtScopeEnd(const std::string &p) : path(p) {}
  ~UnlinkAtScopeEnd() { std::remove(path.c_str()); }

private:
  UnlinkAtScopeEnd(const UnlinkAtScopeEnd &);
  UnlinkAtScopeEnd &operator=(const UnlinkAtScopeEnd &);
};

} // namespace

std::string createDirectiveIndexTestFile() {
  std::string path = "/tmp/test_nginx_file";
  std::ofstream out(path.c_str());

  if (!out)
    throw std::runtime_error("File creation failed");

  out << "events {}\n"
      << "http {\n"
      << "    root /home;\n"
      << "}\n";

  out.close();
  return path;
}

void testDirective_index(bool skip) {
  if (skip)
    return;

  const std::string path = createDirectiveIndexTestFile();
  UnlinkAtScopeEnd cleanup(path);

  Config *config = Parser(path.c_str()).parse();
  if (!config) {
    throw std::runtime_error("testDirective_index: config null");
  }

  assertStrEquals("/home", config->shared_config->root);

  delete config;
}
