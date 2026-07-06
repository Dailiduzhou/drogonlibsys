#pragma once

#include <drogon/HttpController.h>

namespace libsys {

// 全文搜索路由: GET /api/search?q=<keyword>&offset=&limit=
// 使用 PostgreSQL tsvector + GIN 倒排索引
class SearchController : public drogon::HttpController<SearchController> {
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(SearchController::search, "/api/search", Get);
  METHOD_LIST_END

  void search(const drogon::HttpRequestPtr &req,
              std::function<void(const drogon::HttpResponsePtr &)> &&cb);
};

} // namespace libsys
