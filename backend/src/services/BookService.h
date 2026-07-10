#pragma once

#include "libsys/models/Book.h"
#include <cstdint>
#include <drogon/plugins/Plugin.h>
#include <optional>
#include <vector>

namespace libsys {

// 图书服务: CRUD + Redis 缓存 + Singleflight 防缓存击穿
// 参见 AGENTS.md 2.2 图书管理 + 2.4 高并发性能优化
class BookService : public drogon::Plugin<BookService> {
public:
  void initAndStart(const Json::Value &config) override;
  void shutdown() override;

  // 查询图书详情 (Singleflight + Redis 缓存)
  std::optional<Book> getBook(int64_t id);

  std::vector<Book> listBooks(int64_t offset, int64_t limit);

  int64_t createBook(const Book &b);
  bool updateBook(const Book &b);
  bool deleteBook(int64_t id);

  // JSON 序列化 (供缓存与响应复用)
  static std::string bookKey(int64_t id);
  static bool invalidateBookCaches(int64_t id);

private:
  static bool invalidateSearchCaches();
};

} // namespace libsys
