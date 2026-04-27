#include "common.h"
#include "config/config.hpp"
#include "config/config_mock.hpp"
#include "logger/log.hpp"
#include "parser/scanner.hpp"
#include <iostream>

int main() {
  // if (argc < 2) {
  //   Logger::fatal("Usage: %s <configuration_file>", argv[0]);
  // }

  // Scanner scanner;
  // if (scanner.scan(argv[1]) != 0) {
  //   return (EXIT_FAILURE);
  // }

  Config config;
  setup_mock_config(config);
  std::cout << config.toString() << std::endl;

  return (EXIT_SUCCESS);
}
