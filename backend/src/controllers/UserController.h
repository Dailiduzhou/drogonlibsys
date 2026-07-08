#pragma once

#include "filters/JwtAuthFilter.h"
#include <drogon/HttpController.h>

namespace libsys {

using drogon::Delete;
using drogon::Get;

// 用户管理路由 (管理员维护): 均受 JwtAuthFilter 保护
//   GET    /api/users         用户列表
//   GET    /api/users/{id}    用户详情
//   DELETE /api/users/{id}    删除用户 (loan_records 级联清理)
class UserController : public drogon::HttpController<UserController> {
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(UserController::list, "/api/users", Get,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(UserController::get, "/api/users/{1}", Get,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(UserController::remove, "/api/users/{1}", Delete,
                "libsys::JwtAuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&cb);
  void get(const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&cb,
           int64_t id);
  void remove(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&cb,
              int64_t id);
};

} // namespace libsys
