#include "logger/log.hpp"
#include <cassert>
#include <string>

void assertStrEquals(const std::string &wanted, const std::string &got,
                     const std::string &msg) {
  if (wanted != got) {
    Logger::error("%s\nWanted: %s\nGot: %s", msg.c_str(), wanted.c_str(),
                  got.c_str());
  }
}

void assertStrEquals(const std::string &wanted, const std::string &got) {
  if (wanted != got) {
    Logger::error("Wanted: `%s` Got: `%s`", wanted.c_str(), got.c_str());
  }
}
