#pragma once

#include "libsys/models/Book.h"
#include "libsys/models/LoanRecord.h"
#include "libsys/models/User.h"
#include <optional>
#include <string>
#include <vector>

namespace libsys {

// PostgreSQL 数据访问层 (基于 Drogon ORM)
class PgClient {
public:
  static void init(const std::string &host, int port, const std::string &dbname,
                   const std::string &user, const std::string &password,
                   int connNumber);

  // ---- 用户 ----
  static std::optional<User> findUserByName(const std::string &username);
  static std::optional<User> findUserById(int64_t id);
  static std::vector<User> listUsers(int offset, int limit);
  static bool createUser(const std::string &username,
                         const std::string &passwordHash,
                         const std::string &role);
  static bool deleteUser(int64_t id);
  // 角色为 admin 的用户数 (用于防止删除最后一个管理员)
  static int64_t countAdmins();

  // ---- 图书 CRUD ----
  static std::optional<Book> findBookById(int64_t id);
  static std::vector<Book> listBooks(int offset, int limit);
  static int64_t createBook(const Book &b);
  static bool updateBook(const Book &b);
  static bool deleteBook(int64_t id);

  // ---- 出借记录 CRUD ----
  static std::optional<LoanRecord> findLoanRecordById(int64_t id);
  static std::vector<LoanRecord> listLoanRecords(int offset, int limit);
  static std::vector<LoanRecord> listLoanRecordsByUser(int64_t userId,
                                                       int offset, int limit);
  static std::vector<LoanRecord>
  listActiveLoanRecordsByUser(int64_t userId, int offset, int limit);
  // 该用户未还书数量 (status='borrowed'). DB 异常返回 nullopt.
  static std::optional<int64_t> countActiveLoanRecordsByUser(int64_t userId);
  static int64_t createLoanRecord(const LoanRecord &record);
  static bool updateLoanRecord(const LoanRecord &record);
  static bool deleteLoanRecord(int64_t id);

  // ---- 借书 / 还书 ----
  static BorrowBookResult borrowBook(int64_t bookId, int64_t userId);
  static ReturnBookResult returnBook(int64_t bookId, int64_t userId);

  // ---- 全文检索 ----
  static std::vector<Book> search(const std::string &query, int offset,
                                  int limit);
};

} // namespace libsys
