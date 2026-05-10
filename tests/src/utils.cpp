#include "logger/log.hpp"
#include <cassert>
#include <cstdlib>
#include <ctime>
#include <sstream>
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

std::string make_temp_path(const char *prefix) {
  static bool seeded = false;
  if (!seeded) {
    std::srand(static_cast<unsigned>(std::time(0)));
    seeded = true;
  }
  std::ostringstream os;
  os << "/tmp/" << prefix << '_' << std::rand() << '_' << std::time(0);
  return os.str();
}
