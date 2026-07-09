#include "controllers/UserController.h"
#include "libsys/models/ApiResponse.h"
#include "services/UserService.h"

#include <cstdlib>
#include <json/json.h>

namespace libsys {

namespace {
int64_t parseInt64Param(const drogon::HttpRequestPtr &req,
                        const std::string &name) {
  const auto value = req->getParameter(name);
  if (value.empty()) {
    return 0;
  }
  return std::strtoll(value.c_str(), nullptr, 10);
}

// 序列化用户 (不暴露 password 哈希). activeLoans 为该用户未还书数量.
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

int64_t currentUserId(const drogon::HttpRequestPtr &req) {
  return req->getAttributes()->get<int64_t>("userId");
}

std::string currentRole(const drogon::HttpRequestPtr &req) {
  return req->getAttributes()->get<std::string>("role");
}

bool isAdmin(const drogon::HttpRequestPtr &req) {
  return currentRole(req) == "admin";
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

  // 禁止删除仍有未还书 (status='borrowed') 的用户:
  //   - 查询失败 (nullopt) -> fail-closed, 返回 500 阻止删除
  //   - 数量 > 0          -> 409, 提示先归还/管理员手动改状态
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
