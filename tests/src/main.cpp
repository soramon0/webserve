#include "logger/log.hpp"
#include "parser/parser.test.hpp"
#include "request/request-line.test.hpp"

int main() {
  testDirective_index(false);
  testRequestLine(false);
  Logger::info("Finished tests.");
  return 0;
}
