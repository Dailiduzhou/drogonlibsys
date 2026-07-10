#pragma once

#include "filters/JwtAuthFilter.h"
#include <cstdint>
#include <drogon/HttpController.h>

namespace libsys {

using drogon::Delete;
using drogon::Get;
using drogon::Post;
using drogon::Put;

// 出借记录与借还书路由:
//   GET    /api/loans                列表 [auth]
//   GET    /api/loans/{id}           详情 [auth]
//   POST   /api/loans                新增记录(管理员维护) [auth]
//   PUT    /api/loans/{id}           更新记录(管理员维护) [auth]
//   DELETE /api/loans/{id}           删除记录(管理员维护) [auth]
//   POST   /api/books/{id}/borrow    借书 [auth]
//   POST   /api/books/{id}/return    还书 [auth]
class LoanController : public drogon::HttpController<LoanController> {
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(LoanController::list, "/api/loans", Get,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(LoanController::get, "/api/loans/{1}", Get,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(LoanController::create, "/api/loans", Post,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(LoanController::update, "/api/loans/{1}", Put,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(LoanController::remove, "/api/loans/{1}", Delete,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(LoanController::borrow, "/api/books/{1}/borrow", Post,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(LoanController::returnBook, "/api/books/{1}/return", Post,
                "libsys::JwtAuthFilter");
  METHOD_LIST_END

  void list(const drogon::HttpRequestPtr &req,
            std::function<void(const drogon::HttpResponsePtr &)> &&cb);
  void get(const drogon::HttpRequestPtr &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&cb,
           int64_t id);
  void create(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&cb);
  void update(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&cb,
              int64_t id);
  void remove(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&cb,
              int64_t id);
  void borrow(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&cb,
              int64_t bookId);
  void returnBook(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                  int64_t bookId);
};

} // namespace libsys
