#include "libsys/models/ApiResponse.h"

#include <drogon/HttpResponse.h>

namespace libsys {

drogon::HttpResponsePtr ApiResponse::ok(const Json::Value &data) {
  Json::Value root;
  root["code"] = 0;
  root["msg"] = "ok";
  root["data"] = data;
  auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
  resp->setStatusCode(drogon::k200OK);
  return resp;
}

drogon::HttpResponsePtr ApiResponse::fail(int code, const std::string &msg) {
  Json::Value root;
  root["code"] = code;
  root["msg"] = msg;
  root["data"] = Json::nullValue;
  auto resp = drogon::HttpResponse::newHttpJsonResponse(root);
  if (code == 401)
    resp->setStatusCode(drogon::k401Unauthorized);
  else if (code == 403)
    resp->setStatusCode(drogon::k403Forbidden);
  else if (code == 404)
    resp->setStatusCode(drogon::k404NotFound);
  else if (code >= 400 && code < 500)
    resp->setStatusCode(drogon::k400BadRequest);
  else
    resp->setStatusCode(drogon::k500InternalServerError);
  return resp;
}

} // namespace libsys
