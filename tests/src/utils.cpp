#include <cassert>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <string>

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
