#include "services/SearchService.h"
#include "libsys/utils/PgClient.h"
#include "libsys/utils/RedisClient.h"
#include "libsys/utils/Singleflight.h"

#include <json/json.h>

namespace libsys {

namespace {
constexpr int kSearchCacheTtl = 60; // 搜索结果缓存 1 分钟

std::string searchKey(const std::string &q, int offset, int limit) {
  return "search:" + q + ":" + std::to_string(offset) + ":" +
         std::to_string(limit);
}
} // namespace

std::vector<Book> SearchService::search(const std::string &query, int offset,
                                        int limit) {
  std::string key = searchKey(query, offset, limit);

  // Singleflight 合并相同关键词的并发搜索请求, 防缓存击穿
  return Singleflight<std::vector<Book>>::do(key, [&]() -> std::vector<Book> {
    // TODO: 命中 Redis 缓存时直接返回; 当前简化为直接查 PG (GIN 倒排索引)
    auto books = PgClient::search(query, offset, limit);
    // 缓存序列化结果可在此写入 Redis (kSearchCacheTtl)
    return books;
  });
}

} // namespace libsys
