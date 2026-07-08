#include <drogon/drogon.h>

#include "libsys/utils/AppConfig.h"

#include <aws/core/Aws.h>
#include <exception>
#include <iostream>

int main() {
  Aws::SDKOptions options;
  Aws::InitAPI(options);
  try {
    const auto config = libsys::loadAppConfig();

    LOG_INFO << "Loaded config from " << config.filePath;
    LOG_INFO << "drogonlibsys starting on port 8080 ...";

    drogon::app().run();
    Aws::ShutdownAPI(options);
    return 0;
  } catch (const std::exception &e) {
    Aws::ShutdownAPI(options);
    std::cerr << "Startup failed: " << e.what() << '\n';
    return 1;
  }
}
