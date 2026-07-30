#include "config/config.hpp"
#include "logger/log.hpp"
#include "parser/parser.hpp"
#include "server/Webserv.hpp"
#include <iostream>

int main(int ac, char *av[]) {
  try {
    if (DEBUG) {
      Logger::setLevel(Logger::LOG_DEBUG);
    }

    if (ac > 2)
      Logger::fatal("Usage: %s <configuration_file>", av[0]);
    
    std::string file_path = ac == 2 ? av[1] : "nginx/webserv.conf";
    Config *config = Parser(file_path.c_str()).parse();
    if (config == NULL)
      return (1);
    
    Webserv server(*config);
    server.start();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }

  return (EXIT_SUCCESS);
}
