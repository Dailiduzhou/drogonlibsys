#include "services/BookService.h"
#include "libsys/utils/PgClient.h"
#include "libsys/utils/RedisClient.h"
#include "libsys/utils/Singleflight.h"

#include <json/json.h>
#include <sstream>

namespace libsys {

namespace {
constexpr int kCacheTtl = 300; // 图书详情缓存 5 分钟

Json::Value bookToJson(const Book &b) {
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

std::optional<Book> jsonToBook(const std::string &s) {
  Json::CharReaderBuilder rb;
  Json::Value root;
  std::string errs;
  std::istringstream in(s);
  if (!Json::parseFromStream(rb, in, &root, &errs))
    return std::nullopt;
  Book b;
  b.id = root["id"].asInt64();
  b.title = root["title"].asString();
  b.author = root["author"].asString();
  b.description = root["description"].asString();
  b.coverKey = root["coverKey"].asString();
  b.stock = root["stock"].asInt();
  b.createdAt = root["createdAt"].asString();
  b.updatedAt = root["updatedAt"].asString();
  return b;
}

std::string serialize(const Book &b) {
  Json::StreamWriterBuilder wb;
  return Json::writeString(wb, bookToJson(b));
}
} // namespace

void BookService::initAndStart(const Json::Value &config) { (void)config; }

void BookService::shutdown() {}

std::string BookService::bookKey(int64_t id) {
  return "book:" + std::to_string(id);
}

bool BookService::invalidateSearchCaches() {
  return RedisClient::delByPrefix("search:");
}

bool BookService::invalidateBookCaches(int64_t id) {
  const bool bookCacheOk = RedisClient::del(bookKey(id));
  const bool searchCacheOk = invalidateSearchCaches();
  return bookCacheOk && searchCacheOk;
}

std::optional<Book> BookService::getBook(int64_t id) {
  std::string key = bookKey(id);

  // 1. 查 Redis 缓存
  auto cached = RedisClient::get(key);
  if (cached) {
    return jsonToBook(*cached);
  }

  // 2. 缓存未命中 -> Singleflight 合并回源请求, 仅一个请求查 PG
  auto opt = Singleflight<std::optional<Book>>::execute(
      key, [&]() -> std::optional<Book> {
        auto book = PgClient::findBookById(id);
        if (book) {
          RedisClient::set(key, serialize(*book), kCacheTtl);
        }
        return book;
      });

  return opt;
}

std::vector<Book> BookService::listBooks(int offset, int limit) {
  return PgClient::listBooks(offset, limit);
}

int64_t BookService::createBook(const Book &b) {
  auto id = PgClient::createBook(b);
  if (id > 0) {
    if (!invalidateSearchCaches()) {
      LOG_WARN << "failed to invalidate search caches after creating book "
               << id;
    }
  }
  return id;
}

bool BookService::updateBook(const Book &b) {
  bool ok = PgClient::updateBook(b);
  if (ok) {
    if (!invalidateBookCaches(b.id)) {
      LOG_WARN << "failed to invalidate caches after updating book " << b.id;
    }
  }
  return ok;
}

bool BookService::deleteBook(int64_t id) {
  bool ok = PgClient::deleteBook(id);
  if (ok) {
    if (!invalidateBookCaches(id)) {
      LOG_WARN << "failed to invalidate caches after deleting book " << id;
    }
  }
  return ok;
}

} // namespace libsys
