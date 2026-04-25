#include "config/config.hpp"
#include "config/config_mock.hpp"
#include <iostream>

int main() {
  Config config;
  setup_mock_config(config);
  std::cout << config.toString() << std::endl;
  return 0;
}
