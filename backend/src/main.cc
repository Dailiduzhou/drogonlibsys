#include <drogon/drogon.h>

#include "libsys/utils/AppConfig.h"

#include <exception>
#include <iostream>
// 强制将 yylex 符号设为默认可见，允许 libfl.so 链接
extern "C" __attribute__((visibility("default"))) int yylex() { return 0; }
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
