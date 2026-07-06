#pragma once

#include "libsys/models/Book.h"
#include "libsys/models/User.h"
#include <optional>
#include <string>
#include <vector>

namespace libsys {

// PostgreSQL 数据访问层 (基于 libpqxx)
class PgClient {
public:
  static void init();

  // ---- 用户 ----
  static std::optional<User> findUserByName(const std::string &username);
  static bool createUser(const std::string &username,
                         const std::string &passwordHash,
                         const std::string &role);

  // ---- 图书 CRUD ----
  static std::optional<Book> findBookById(int64_t id);
  static std::vector<Book> listBooks(int offset, int limit);
  static int64_t createBook(const Book &b);
  static bool updateBook(const Book &b);
  static bool deleteBook(int64_t id);

  // ---- 全文检索 (tsquery) ----
  // 使用 books.tsv GIN 倒排索引, 按权重排序
  static std::vector<Book> search(const std::string &query, int offset,
                                  int limit);
};

} // namespace libsys
