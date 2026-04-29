#include "common.h"
#include "logger/log.hpp"
#include "parser/parser.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    Logger::fatal("Usage: %s <configuration_file>", argv[0]);
  }

  Parser parser;
  if (parser.parseConfig(argv[1]) != 0) {
    return (EXIT_FAILURE);
  }

  return (EXIT_SUCCESS);
}
