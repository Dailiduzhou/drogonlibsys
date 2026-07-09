#include "controllers/BookController.h"
#include "libsys/utils/HttpHelpers.h"
#include "libsys/models/ApiResponse.h"
#include "services/BookService.h"
#include "services/LoanService.h"
#include "services/OssService.h"

#include <drogon/MultiPart.h>
#include <exception>
#include <json/json.h>

namespace libsys {

namespace {

std::string fileMimeType(const drogon::HttpFile &file) {
  switch (file.getContentType()) {
  case drogon::CT_IMAGE_APNG:
    return "image/apng";
  case drogon::CT_IMAGE_AVIF:
    return "image/avif";
  case drogon::CT_IMAGE_BMP:
    return "image/bmp";
  case drogon::CT_IMAGE_GIF:
    return "image/gif";
  case drogon::CT_IMAGE_ICNS:
    return "image/icns";
  case drogon::CT_IMAGE_JPG:
    return "image/jpeg";
  case drogon::CT_IMAGE_JP2:
    return "image/jp2";
  case drogon::CT_IMAGE_PNG:
    return "image/png";
  case drogon::CT_IMAGE_SVG_XML:
    return "image/svg+xml";
  case drogon::CT_IMAGE_TIFF:
    return "image/tiff";
  case drogon::CT_IMAGE_WEBP:
    return "image/webp";
  case drogon::CT_IMAGE_X_MNG:
    return "image/x-mng";
  case drogon::CT_IMAGE_X_TGA:
    return "image/x-tga";
  case drogon::CT_IMAGE_XICON:
    return "image/x-icon";
  default:
    return "application/octet-stream";
  }
}

} // namespace

void BookController::list(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  int64_t offset = parseInt64Param(req, "offset");
  int64_t limit = parseInt64Param(req, "limit");
  if (limit <= 0)
    limit = 20;

  auto *svc = drogon::app().getPlugin<BookService>();
  auto books = svc->listBooks(offset, limit);
  Json::Value data(Json::arrayValue);
  for (const auto &b : books)
    data.append(bookJson(b));
  cb(ApiResponse::ok(data));
}

void BookController::get(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  (void)req;
  auto *svc = drogon::app().getPlugin<BookService>();
  auto book = svc->getBook(id);
  if (!book) {
    cb(ApiResponse::fail(404, "book not found"));
    return;
  }
  cb(ApiResponse::ok(bookJson(*book)));
}

void BookController::create(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  if (!isAdmin(req)) {
    cb(ApiResponse::fail(403, "admin only"));
    return;
  }

  auto json = req->getJsonObject();
  if (!json) {
    cb(ApiResponse::fail(400, "json body required"));
    return;
  }
  Book b;
  b.title = json->get("title", "").asString();
  b.author = json->get("author", "").asString();
  b.description = json->get("description", "").asString();
  b.coverKey = json->get("coverKey", "").asString();
  b.stock = json->get("stock", 0).asInt64();
  if (b.title.empty() || b.author.empty()) {
    cb(ApiResponse::fail(400, "title and author required"));
    return;
  }
  if (b.stock < 0) {
    cb(ApiResponse::fail(400, "stock must not be negative"));
    return;
  }
  auto *svc = drogon::app().getPlugin<BookService>();
  auto id = svc->createBook(b);
  Json::Value data;
  data["id"] = (Json::Int64)id;
  cb(ApiResponse::ok(data));
}

void BookController::update(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  if (!isAdmin(req)) {
    cb(ApiResponse::fail(403, "admin only"));
    return;
  }

  auto json = req->getJsonObject();
  if (!json) {
    cb(ApiResponse::fail(400, "json body required"));
    return;
  }

  auto *svc = drogon::app().getPlugin<BookService>();
  auto existing = svc->getBook(id);
  if (!existing) {
    cb(ApiResponse::fail(404, "book not found"));
    return;
  }

  if (json->isMember("title"))
    existing->title = (*json)["title"].asString();
  if (json->isMember("author"))
    existing->author = (*json)["author"].asString();
  if (json->isMember("description"))
    existing->description = (*json)["description"].asString();
  if (json->isMember("coverKey"))
    existing->coverKey = (*json)["coverKey"].asString();
  if (json->isMember("stock")) {
    auto newStock = (*json)["stock"].asInt64();
    if (newStock < 0) {
      cb(ApiResponse::fail(400, "stock must not be negative"));
      return;
    }
    existing->stock = newStock;
  }

  if (!svc->updateBook(*existing)) {
    cb(ApiResponse::fail(500, "update failed"));
    return;
  }
  cb(ApiResponse::ok());
}

void BookController::remove(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  if (!isAdmin(req)) {
    cb(ApiResponse::fail(403, "admin only"));
    return;
  }

  auto *svc = drogon::app().getPlugin<BookService>();
  auto book = svc->getBook(id);
  if (!book) {
    cb(ApiResponse::fail(404, "book not found"));
    return;
  }

  auto *loanSvc = drogon::app().getPlugin<LoanService>();
  auto active = loanSvc->countActiveLoansByBook(id);
  if (!active) {
    cb(ApiResponse::fail(500, "cannot verify active loans"));
    return;
  }
  if (*active > 0) {
    cb(ApiResponse::fail(409,
                         "book has " + std::to_string(*active) +
                             " unreturned loan(s); return them before deleting"));
    return;
  }

  if (!svc->deleteBook(id)) {
    cb(ApiResponse::fail(500, "delete failed"));
    return;
  }

  if (!book->coverKey.empty()) {
    try {
      auto *oss = drogon::app().getPlugin<OssService>();
      oss->deleteCover(book->coverKey);
    } catch (const std::exception &e) {
      LOG_WARN << "failed to remove cover object for deleted book " << id
               << ": " << e.what();
    }
  }
  cb(ApiResponse::ok());
}

void BookController::uploadCover(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  if (!isAdmin(req)) {
    cb(ApiResponse::fail(403, "admin only"));
    return;
  }

  drogon::MultiPartParser parser;
  if (parser.parse(req) != 0 || parser.getFiles().empty()) {
    cb(ApiResponse::fail(400, "no file uploaded"));
    return;
  }
  const auto &files = parser.getFiles();

  auto *svc = drogon::app().getPlugin<BookService>();
  auto book = svc->getBook(id);
  if (!book) {
    cb(ApiResponse::fail(404, "book not found"));
    return;
  }

  const auto &file = files[0];
  std::string key;
  std::string coverUrl;
  const auto oldCoverKey = book->coverKey;
  try {
    auto *oss = drogon::app().getPlugin<OssService>();
    const auto content = file.fileContent();
    key = oss->uploadCover(id, file.getFileName(), fileMimeType(file),
                           std::string(content.data(), content.size()));
    coverUrl = oss->coverUrl(key);
  } catch (const std::exception &e) {
    cb(ApiResponse::fail(500, std::string("cover upload failed: ") + e.what()));
    return;
  }

  book->coverKey = key;
  if (!svc->updateBook(*book)) {
    try {
      auto *oss = drogon::app().getPlugin<OssService>();
      oss->deleteCover(key);
    } catch (const std::exception &e) {
      LOG_WARN << "failed to rollback uploaded cover for book " << id << ": "
               << e.what();
    }
    cb(ApiResponse::fail(500, "cover metadata update failed"));
    return;
  }

  if (!oldCoverKey.empty() && oldCoverKey != key) {
    try {
      auto *oss = drogon::app().getPlugin<OssService>();
      oss->deleteCover(oldCoverKey);
    } catch (const std::exception &e) {
      LOG_WARN << "failed to remove old cover object for book " << id << ": "
               << e.what();
    }
  }

  Json::Value data;
  data["coverKey"] = key;
  data["coverUrl"] = coverUrl;
  cb(ApiResponse::ok(data));
}

} // namespace libsys
