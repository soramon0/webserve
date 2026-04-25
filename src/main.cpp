#include "logger/log.hpp"
#include "parser/scanner.hpp"
#include "common.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    Logger::fatal("Usage: %s <configuration_file>", argv[0]);
  }

  Scanner scanner;
  if (scanner.scan(argv[1]) != 0) {
    return (EXIT_FAILURE);
  }

  return (EXIT_SUCCESS);
}
