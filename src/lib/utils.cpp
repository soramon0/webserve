#include "utils.hpp"
#include "logger/log.hpp"
#include <sstream>

size_t countDigits(long n) {
  if (n == 0)
    return 1;
  size_t count = 0;
  n = std::abs(n);
  while (n > 0) {
    n /= 10;
    count++;
  }
  return count;
}

void reportError(const std::string &src, size_t row, size_t start,
                 const std::string &msg) {
  std::ostringstream oss;

  // [Error] "row:column: line_content"
  // logger prefix
  size_t loggerPrefix = 11;
  oss << row << ":" << start + 1 << "| " << src << "\n";
  size_t prefixLen = countDigits(row) + countDigits(start + 1) + loggerPrefix;
  std::string underline(prefixLen + start, ' ');
  underline += "^ " + msg;
  oss << underline;

  Logger::error("%s", oss.str().c_str());
}
