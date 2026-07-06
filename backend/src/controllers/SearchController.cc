#include "controllers/SearchController.h"
#include "libsys/models/ApiResponse.h"
#include "services/SearchService.h"

#include <json/json.h>

namespace libsys {

namespace {
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
  int offset = std::atoi(req->getParameter("offset").c_str());
  int limit = std::atoi(req->getParameter("limit").c_str());
  if (limit <= 0)
    limit = 20;
  if (q.empty()) {
    cb(ApiResponse::fail(400, "query param 'q' required"));
    return;
  }

  SearchService svc;
  auto books = svc.search(q, offset, limit);
  Json::Value data(Json::arrayValue);
  for (const auto &b : books)
    data.append(bookJson(b));
  cb(ApiResponse::ok(data));
}

} // namespace libsys
