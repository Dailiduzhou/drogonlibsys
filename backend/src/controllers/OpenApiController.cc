#include "controllers/OpenApiController.h"

#include <drogon/HttpResponse.h>
#include <drogon/HttpAppFramework.h>
#include <fstream>
#include <sstream>

namespace libsys {

void OpenApiController::getSpec(
    const drogon::HttpRequestPtr &,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  std::ifstream ifs("openapi.yaml");
  if (!ifs.is_open()) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k404NotFound);
    resp->setBody("openapi.yaml not found");
    cb(resp);
    return;
  }
  std::ostringstream ss;
  ss << ifs.rdbuf();
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setContentTypeString("application/x-yaml");
  resp->setBody(ss.str());
  cb(resp);
}

} // namespace libsys
