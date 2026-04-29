#include "parser.hpp"
#include "logger/log.hpp"
#include "parser/scanner.hpp"
#include <cstring>
#include <fstream>

ssize_t Parser::parseConfig(char *filepath) {
  std::ifstream configFile(filepath);

  if (!configFile) {
    Logger::error("Could not open configuration file '%s': %s", filepath,
                  std::strerror(errno));
    return -1;
  }

  Scanner scanner;
  if (scanner.tokenize(configFile) != 0) {
    return -1;
  }
  scanner.printTokens();

  return (0);
}
