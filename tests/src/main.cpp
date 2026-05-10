#include "logger/log.hpp"
#include "parser/parser.tests.hpp"

int main() {
  testDirective_index(false);
  Logger::info("Finished tests.");
  return 0;
}
