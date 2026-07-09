#include "services/SearchService.h"
#include "libsys/utils/PgClient.h"
#include "libsys/utils/RedisClient.h"
#include "libsys/utils/Singleflight.h"

#include <json/json.h>
#include <sstream>

namespace libsys {

namespace {
constexpr int kSearchCacheTtl = 60;

std::string searchKey(const std::string &q, int64_t offset, int64_t limit) {
  return "search:" + q + ":" + std::to_string(offset) + ":" +
         std::to_string(limit);
}

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

std::string serialize(const std::vector<Book> &books) {
  Json::Value arr(Json::arrayValue);
  for (const auto &b : books)
    arr.append(bookToJson(b));
  Json::StreamWriterBuilder wb;
  return Json::writeString(wb, arr);
}

std::vector<Book> deserialize(const std::string &s) {
  Json::CharReaderBuilder rb;
  Json::Value root;
  std::string errs;
  std::istringstream in(s);
  if (!Json::parseFromStream(rb, in, &root, &errs))
    return {};
  std::vector<Book> out;
  out.reserve(root.size());
  for (const auto &v : root) {
    Book b;
    b.id = v["id"].asInt64();
    b.title = v["title"].asString();
    b.author = v["author"].asString();
    b.description = v["description"].asString();
    b.coverKey = v["coverKey"].asString();
    b.stock = v["stock"].asInt64();
    b.createdAt = v["createdAt"].asString();
    b.updatedAt = v["updatedAt"].asString();
    out.push_back(std::move(b));
  }
  return out;
}
} // namespace

void SearchService::initAndStart(const Json::Value &config) { (void)config; }

void SearchService::shutdown() {}

std::vector<Book> SearchService::search(const std::string &query,
                                        int64_t offset, int64_t limit) {
  std::string key = searchKey(query, offset, limit);

  auto cached = RedisClient::get(key);
  if (cached) {
    return deserialize(*cached);
  }

  return Singleflight<std::vector<Book>>::execute(
      key, [&]() -> std::vector<Book> {
        auto books = PgClient::search(query, offset, limit);
        RedisClient::set(key, serialize(books), kSearchCacheTtl);
        return books;
      });
}

} // namespace libsys
