#include "common.h"
#include "config/config.hpp"
#include "config/config_mock.hpp"
#include "logger/log.hpp"
#include "parser/scanner.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    Logger::fatal("Usage: %s <configuration_file>", argv[0]);
  }

  Scanner scanner;
  if (scanner.scan(argv[1]) != 0) {
    return (EXIT_FAILURE);
  }

  for (std::vector<Token>::const_iterator it = scanner.tokens.begin();
       it != scanner.tokens.end(); it++) {
    std::cout << it->toString() << std::endl;
  }

  return (EXIT_SUCCESS);
}
