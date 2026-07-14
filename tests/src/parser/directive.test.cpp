#include "../utils.hpp"
#include "common.h"
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

TEST_CASE("Parser - client_max_body_size with units") {
  ScopedFile f("body_size_units.test",
               "events {}\nhttp {\n"
               "  client_max_body_size 16kib;\n"
               "  server {\n"
               "    listen 80;\n"
               "    location / {\n"
               "      client_max_body_size 2mib;\n"
               "    }\n"
               "  }\n"
               "}\n");

  Config *config = Parser(f.path.c_str()).parse();
  REQUIRE(config != nullptr);
  CHECK(config->shared_config->client_max_body_size == KIB(16));
  REQUIRE(config->servers.size() == 1);
  REQUIRE(config->servers[0]->locations.count("/") == 1);
  CHECK(config->servers[0]->locations["/"].shared_config->client_max_body_size ==
        MIB(2));
  delete config;
}
