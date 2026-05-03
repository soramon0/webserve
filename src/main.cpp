#include "common.h"
#include "config/config.hpp"
#include "logger/log.hpp"
#include "parser/parser.hpp"
#include "server/Webserv.hpp"
#include <iostream>

int main(int ac, char *av[]) {
	try {
		if (ac > 2)
			throw std::invalid_argument("Usage: ./webserv [config_file]");

		std::string config_file = (ac == 2) ? av[1] : "../nginx/nginx.conf";

		Config config;


		Webserv websrv(config);
		websrv.start();

	}
	catch(const std::exception& e) {
		std::cerr << e.what() << '\n';
	}

	return (EXIT_SUCCESS);
}
