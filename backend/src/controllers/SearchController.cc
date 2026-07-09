#include "controllers/SearchController.h"
#include "libsys/models/ApiResponse.h"
#include "services/SearchService.h"

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

Json::Value bookJson(const Book &b) {
  Json::Value v;
  v["id"] = (Json::Int64)b.id;
  v["title"] = b.title;
  v["author"] = b.author;
  v["description"] = b.description;
  v["coverKey"] = b.coverKey;
  v["stock"] = b.stock;
  v["createdAt"] = b.createdAt;
  v["updatedAt"] = b.updatedAt;
  return v;
}
} // namespace

void SearchController::search(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  std::string q = req->getParameter("q");
  int64_t offset = parseInt64Param(req, "offset");
  int64_t limit = parseInt64Param(req, "limit");
  if (limit <= 0)
    limit = 20;
  if (q.empty()) {
    cb(ApiResponse::fail(400, "query param 'q' required"));
    return;
  }

  auto *svc = drogon::app().getPlugin<SearchService>();
  auto books = svc->search(q, offset, limit);
  Json::Value data(Json::arrayValue);
  for (const auto &b : books)
    data.append(bookJson(b));
  cb(ApiResponse::ok(data));
}

} // namespace libsys
