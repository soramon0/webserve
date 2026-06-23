#include "../utils.hpp"
#include "config/config.hpp"
#include "logger/log.hpp"
#include "parser.test.hpp"
#include "parser/parser.hpp"

void testDirective_index(bool skip) {
  if (skip) {
    Logger::info("skipping testDirective_index...");
    return;
  }

  ScopedFile f("index.test", "events {}\nhttp {root /home;}");

  Config *config = Parser(f.path.c_str()).parse();
  if (!config) {
    throw std::runtime_error("testDirective_index: config null");
  }

  assertStrEquals("/home", config->shared_config->root);

  delete config;
}
