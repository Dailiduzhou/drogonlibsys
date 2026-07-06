#pragma once

#include "filters/JwtAuthFilter.h"
#include <drogon/HttpController.h>

namespace libsys {

using drogon::Delete;
using drogon::Get;
using drogon::Post;
using drogon::Put;

// 图书路由 (RESTful): 写操作受 JwtAuthFilter 保护
//   GET    /api/books              列表
//   GET    /api/books/{id}         详情 (Singleflight + Redis 缓存)
//   POST   /api/books              新增 [auth]
//   PUT    /api/books/{id}         更新 [auth]
//   DELETE /api/books/{id}         删除 [auth]
//   POST   /api/books/{id}/cover   封面上传 [auth]
class BookController : public drogon::HttpController<BookController> {
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(BookController::list, "/api/books", Get);
  ADD_METHOD_TO(BookController::get, "/api/books/{1}", Get);
  ADD_METHOD_TO(BookController::create, "/api/books", Post,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(BookController::update, "/api/books/{1}", Put,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(BookController::remove, "/api/books/{1}", Delete,
                "libsys::JwtAuthFilter");
  ADD_METHOD_TO(BookController::uploadCover, "/api/books/{1}/cover", Post,
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
  void uploadCover(const drogon::HttpRequestPtr &req,
                   std::function<void(const drogon::HttpResponsePtr &)> &&cb,
                   int64_t id);
};

} // namespace libsys
