#include "controllers/BookController.h"
#include "filters/JwtAuthFilter.h"
#include "libsys/models/ApiResponse.h"
#include "services/BookService.h"
#include "services/OssService.h"

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

void BookController::list(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  int offset = std::atoi(req->getParameter("offset").c_str());
  int limit = std::atoi(req->getParameter("limit").c_str());
  if (limit <= 0)
    limit = 20;

  BookService svc;
  auto books = svc.listBooks(offset, limit);
  Json::Value data(Json::arrayValue);
  for (const auto &b : books)
    data.append(bookJson(b));
  cb(ApiResponse::ok(data));
}

void BookController::get(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  BookService svc;
  auto book = svc.getBook(id);
  if (!book) {
    cb(ApiResponse::fail(404, "book not found"));
    return;
  }
  cb(ApiResponse::ok(bookJson(*book)));
}

void BookController::create(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
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
  b.stock = json->get("stock", 0).asInt();
  if (b.title.empty() || b.author.empty()) {
    cb(ApiResponse::fail(400, "title and author required"));
    return;
  }
  BookService svc;
  auto id = svc.createBook(b);
  Json::Value data;
  data["id"] = (Json::Int64)id;
  cb(ApiResponse::ok(data));
}

void BookController::update(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  auto json = req->getJsonObject();
  if (!json) {
    cb(ApiResponse::fail(400, "json body required"));
    return;
  }
  Book b;
  b.id = id;
  b.title = json->get("title", "").asString();
  b.author = json->get("author", "").asString();
  b.description = json->get("description", "").asString();
  b.coverKey = json->get("coverKey", "").asString();
  b.stock = json->get("stock", 0).asInt();
  BookService svc;
  if (!svc.updateBook(b)) {
    cb(ApiResponse::fail(500, "update failed"));
    return;
  }
  cb(ApiResponse::ok());
}

void BookController::remove(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  BookService svc;
  if (!svc.deleteBook(id)) {
    cb(ApiResponse::fail(500, "delete failed"));
    return;
  }
  cb(ApiResponse::ok());
}

// 封面上传: multipart/form-data -> MinIO -> 更新 books.cover_key
void BookController::uploadCover(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  const auto &files = req->getFiles();
  if (files.empty()) {
    cb(ApiResponse::fail(400, "no file uploaded"));
    return;
  }
  const auto &file = files[0];
  OssService oss;
  std::string key = oss.uploadCover(
      id, file.getFileName(), file.getMimeType(),
      std::string(file.getFileContent().data(), file.getFileContent().size()));

  // 回写数据库 cover_key
  BookService svc;
  auto book = svc.getBook(id);
  if (!book) {
    cb(ApiResponse::fail(404, "book not found"));
    return;
  }
  book->coverKey = key;
  svc.updateBook(*book);

  Json::Value data;
  data["coverKey"] = key;
  data["coverUrl"] = oss.coverUrl(key);
  cb(ApiResponse::ok(data));
}

} // namespace libsys
