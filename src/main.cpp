#include "config/config.hpp"
#include <iostream>

int main() {
  SharedConfig cfg;

  cfg.withRoot("/tmp/wwww")
      .withIndex("index.html")
      .withIndex("index.htm")
      .withAutoIndex(true)
      .withMimetype("jpg", "image/jpeg")
      .withErrorPage(404, "./custom_404.html")
      .withCgi("py", "/usr/bin/python3")
      .withCgi("php", "/usr/bin/php8.4");

  Config config;
  config.assignSharedConfig(cfg.clone());

  std::cout << config.toString() << std::endl;
  return 0;
}
