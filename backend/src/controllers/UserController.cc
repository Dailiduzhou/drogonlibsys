#include "controllers/UserController.h"
#include "libsys/utils/HttpHelpers.h"
#include "libsys/models/ApiResponse.h"
#include "services/UserService.h"

#include <json/json.h>

namespace libsys {

namespace {
Json::Value userJson(const User &u, int64_t activeLoans = 0) {
  Json::Value v;
  v["id"] = (Json::Int64)u.id;
  v["username"] = u.username;
  v["role"] = u.role;
  v["activeLoans"] = (Json::Int64)activeLoans;
  v["createdAt"] = u.createdAt;
  v["updatedAt"] = u.updatedAt;
  return v;
}
} // namespace

void UserController::list(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  if (!isAdmin(req)) {
    cb(ApiResponse::fail(403, "admin only"));
    return;
  }

  int64_t offset = parseInt64Param(req, "offset");
  int64_t limit = parseInt64Param(req, "limit");
  if (limit <= 0)
    limit = 20;

  auto *svc = drogon::app().getPlugin<UserService>();
  auto users = svc->listUsers(offset, limit);
  Json::Value data(Json::arrayValue);
  for (const auto &u : users) {
    auto active = svc->countActiveLoansByUser(u.id);
    data.append(userJson(u, active.value_or(0)));
  }
  cb(ApiResponse::ok(data));
}

void UserController::get(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  if (!isAdmin(req)) {
    cb(ApiResponse::fail(403, "admin only"));
    return;
  }

  auto *svc = drogon::app().getPlugin<UserService>();
  auto user = svc->getUser(id);
  if (!user) {
    cb(ApiResponse::fail(404, "user not found"));
    return;
  }
  auto active = svc->countActiveLoansByUser(id);
  cb(ApiResponse::ok(userJson(*user, active.value_or(0))));
}

void UserController::remove(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  if (!isAdmin(req)) {
    cb(ApiResponse::fail(403, "admin only"));
    return;
  }

  if (id == currentUserId(req)) {
    cb(ApiResponse::fail(400, "cannot delete yourself"));
    return;
  }

  auto *svc = drogon::app().getPlugin<UserService>();
  auto user = svc->getUser(id);
  if (!user) {
    cb(ApiResponse::fail(404, "user not found"));
    return;
  }

  if (user->role == "admin" && svc->countAdmins() <= 1) {
    cb(ApiResponse::fail(409, "cannot delete the last admin"));
    return;
  }

  auto active = svc->countActiveLoansByUser(id);
  if (!active) {
    cb(ApiResponse::fail(500, "cannot verify active loans"));
    return;
  }
  if (*active > 0) {
    cb(ApiResponse::fail(409,
                         "user has " + std::to_string(*active) +
                             " active loan(s); return them before deleting"));
    return;
  }

  if (!svc->deleteUser(id)) {
    cb(ApiResponse::fail(500, "delete failed"));
    return;
  }
  cb(ApiResponse::ok());
}

} // namespace libsys
