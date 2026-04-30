#include "common.h"
#include "config/config.hpp"
#include "logger/log.hpp"
#include "parser/parser.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    Logger::fatal("Usage: %s <configuration_file>", argv[0]);
  }

  Config *config = Parser(argv[1]).parse();
  if (config == NULL) {
    return (EXIT_FAILURE);
  }

  delete config;
  return (EXIT_SUCCESS);
}
