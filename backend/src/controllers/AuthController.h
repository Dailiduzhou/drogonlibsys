#pragma once

#include "filters/JwtAuthFilter.h"
#include <drogon/HttpController.h>

namespace libsys {

using drogon::Delete;
using drogon::Get;
using drogon::Post;
using drogon::Put;

// 鉴权路由: /api/auth/register | /api/auth/login | /api/auth/refresh |
// /api/auth/logout
class AuthController : public drogon::HttpController<AuthController> {
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(AuthController::registerUser, "/api/auth/register", Post);
  ADD_METHOD_TO(AuthController::login, "/api/auth/login", Post);
  ADD_METHOD_TO(AuthController::refresh, "/api/auth/refresh", Post);
  ADD_METHOD_TO(AuthController::logout, "/api/auth/logout", Post,
                "libsys::JwtAuthFilter");
  METHOD_LIST_END

  void registerUser(const drogon::HttpRequestPtr &req,
                    std::function<void(const drogon::HttpResponsePtr &)> &&cb);
  void login(const drogon::HttpRequestPtr &req,
             std::function<void(const drogon::HttpResponsePtr &)> &&cb);
  void refresh(const drogon::HttpRequestPtr &req,
               std::function<void(const drogon::HttpResponsePtr &)> &&cb);
  void logout(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&cb);
};

} // namespace libsys
