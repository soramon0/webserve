#include "common.h"
#include "config/config.hpp"
#include "logger/log.hpp"
#include "parser/parser.hpp"
#include "parser/scanner.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    Logger::fatal("Usage: %s <configuration_file>", argv[0]);
  }

  Scanner scanner;
  if (scanner.scan(argv[1]) != 0) {
    return (EXIT_FAILURE);
  }

  Parser parser(scanner.getTokens());
  Config *config = parser.parse();
  if (config == NULL) {
    return (EXIT_FAILURE);
  }

  delete config;
  return (EXIT_SUCCESS);
}
