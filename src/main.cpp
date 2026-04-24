#include "config/config.hpp"
#include "config/location.hpp"
#include <iostream>

int main() {
  Config config;
  SharedConfig cfg;

  config.withSharedConfig(SharedConfig()
                              .withRoot("/tmp/wwww")
                              .withIndex("index.html")
                              .withIndex("index.htm")
                              .withAutoIndex(true)
                              .withMimetype("jpg", "image/jpeg")
                              .withErrorPage(404, "./custom_404.html")
                              .withCgi("py", "/usr/bin/python3")
                              .withCgi("php", "/usr/bin/php8.4"));

  config.withServer(Server()
                        .withPort(9000)
                        .withInterface("127.0.0.1")
                        .withRedirect(307, "/home"));

  config.withServer(Server()
                        .withPort(8000)
                        .withInterface("127.0.0.1")
                        .withRedirect(303, "/fallback"));

  std::cout << config.toString() << std::endl;
  return 0;
}
