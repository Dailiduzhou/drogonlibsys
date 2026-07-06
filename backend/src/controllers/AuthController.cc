#include "controllers/AuthController.h"
#include "filters/JwtAuthFilter.h"
#include "libsys/models/ApiResponse.h"
#include "services/AuthService.h"

#include <json/json.h>

namespace libsys {

void AuthController::registerUser(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  auto json = req->getJsonObject();
  if (!json || !json->isMember("username") || !json->isMember("password")) {
    cb(ApiResponse::fail(400, "username and password required"));
    return;
  }

  auto *svc = drogon::app().getPlugin<AuthService>();
  auto result = svc->registerUser(json->get("username", "").asString(),
                                  json->get("password", "").asString());
  switch (result.status) {
  case RegisterStatus::InvalidInput:
    cb(ApiResponse::fail(400, "username and password required"));
    return;
  case RegisterStatus::UsernameTaken:
    cb(ApiResponse::fail(409, "username already exists"));
    return;
  case RegisterStatus::Failed:
    cb(ApiResponse::fail(500, "failed to create user"));
    return;
  case RegisterStatus::Success:
    break;
  }

  if (!result.tokens) {
    cb(ApiResponse::fail(500, "failed to issue tokens"));
    return;
  }

  Json::Value data;
  data["accessToken"] = result.tokens->accessToken;
  data["refreshToken"] = result.tokens->refreshToken;
  data["accessExpiresAt"] = (Json::Int64)result.tokens->accessExpiresAt;
  data["refreshExpiresAt"] = (Json::Int64)result.tokens->refreshExpiresAt;
  cb(ApiResponse::ok(data));
}

void AuthController::login(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  auto json = req->getJsonObject();
  if (!json || !json->isMember("username") || !json->isMember("password")) {
    cb(ApiResponse::fail(400, "username and password required"));
    return;
  }
  auto *svc = drogon::app().getPlugin<AuthService>();
  auto pair = svc->login(json->get("username", "").asString(),
                         json->get("password", "").asString());
  if (!pair) {
    cb(ApiResponse::fail(401, "invalid credentials"));
    return;
  }
  Json::Value data;
  data["accessToken"] = pair->accessToken;
  data["refreshToken"] = pair->refreshToken;
  data["accessExpiresAt"] = (Json::Int64)pair->accessExpiresAt;
  data["refreshExpiresAt"] = (Json::Int64)pair->refreshExpiresAt;
  cb(ApiResponse::ok(data));
}

void AuthController::refresh(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  auto json = req->getJsonObject();
  if (!json || !json->isMember("refreshToken")) {
    cb(ApiResponse::fail(400, "refreshToken required"));
    return;
  }
  auto *svc = drogon::app().getPlugin<AuthService>();
  auto pair = svc->refresh(json->get("refreshToken", "").asString());
  if (!pair) {
    cb(ApiResponse::fail(401, "invalid or expired refresh token"));
    return;
  }
  Json::Value data;
  data["accessToken"] = pair->accessToken;
  data["refreshToken"] = pair->refreshToken;
  data["accessExpiresAt"] = (Json::Int64)pair->accessExpiresAt;
  data["refreshExpiresAt"] = (Json::Int64)pair->refreshExpiresAt;
  cb(ApiResponse::ok(data));
}

void AuthController::logout(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  auto auth = req->getHeader("Authorization");
  constexpr const char *prefix = "Bearer ";
  if (auth.size() <= sizeof(prefix) - 1 ||
      auth.compare(0, sizeof(prefix) - 1, prefix) != 0) {
    cb(ApiResponse::fail(400, "missing token"));
    return;
  }
  std::string token = auth.substr(sizeof(prefix) - 1);
  auto *svc = drogon::app().getPlugin<AuthService>();
  svc->logout(token);
  cb(ApiResponse::ok());
}

} // namespace libsys
