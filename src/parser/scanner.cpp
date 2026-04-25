#include "scanner.hpp"
#include "../logger/log.hpp"
#include <cstring>
#include <fstream>

ssize_t Scanner::scan(char *filepath) {
  std::ifstream configFile(filepath);

  if (!configFile) {
    Logger::error("Could not open configuration file '%s': %s", filepath,
                  std::strerror(errno));
    return -1;
  }

  tokenize(configFile);
  return 0;
}

void Scanner::tokenize(std::ifstream &file) {
  std::string line;

  while (std::getline(file, line)) {
    Logger::info("%s", line.c_str());
  }
}
