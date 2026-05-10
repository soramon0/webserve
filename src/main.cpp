#include "common.h"
#include "config/config.hpp"
#include "logger/log.hpp"
#include "parser/parser.hpp"
#include "server/Webserv.hpp"
#include <iostream>

int main(int ac, char *av[]) {
	try {
		if (ac < 2)
			Logger::fatal("Usage: %s <configuration_file>", av[0]);

		Config *config = Parser(av[1]).parse();
    if (config == NULL)
      return (1);
    
		Webserv(*config).start();
    // TODO: Webserv frees config in deconstructor.
    delete config;
	}
	catch(const std::exception& e) {
		std::cerr << e.what() << '\n';
	}

	return (EXIT_SUCCESS);
}
