#include "controllers/SearchController.h"
#include "libsys/utils/HttpHelpers.h"
#include "libsys/models/ApiResponse.h"
#include "services/SearchService.h"

#include <json/json.h>

namespace libsys {

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
