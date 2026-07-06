#include <drogon/drogon.h>

#include "libsys/utils/AppConfig.h"

#include <exception>
#include <iostream>

int main() {
  try {
    const auto config = libsys::loadAppConfig();

    LOG_INFO << "Loaded config from " << config.filePath;
    LOG_INFO << "drogonlibsys starting on port 8080 ...";

    drogon::app().run();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Startup failed: " << e.what() << '\n';
    return 1;
  }
}
