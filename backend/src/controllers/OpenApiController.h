#pragma once

#include <drogon/HttpController.h>

namespace libsys {

using drogon::Get;

class OpenApiController : public drogon::HttpController<OpenApiController> {
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(OpenApiController::getSpec, "/api/openapi.yaml", Get);
  METHOD_LIST_END

  void getSpec(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&cb);
};

} // namespace libsys
