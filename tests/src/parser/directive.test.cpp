#include "../utils.hpp"
#include "config/config.hpp"
#include "doctest.h"
#include "parser/parser.hpp"
#include <string>

TEST_CASE("Parser - index/root directive" * doctest::skip(false)) {
  ScopedFile f("index.test", "events {}\nhttp {root /home;}");

  Config *config = Parser(f.path.c_str()).parse();

  REQUIRE(config != nullptr);

  CHECK(std::string(config->shared_config->root) == "/home");
  delete config;
}
