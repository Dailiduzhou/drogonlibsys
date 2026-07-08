#include "libsys/utils/PgClient.h"

#include <cstdint>
#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>

namespace libsys {

namespace {
drogon::orm::DbClientPtr g_db;

constexpr int kDefaultPageSize = 20;

int normalizedOffset(int offset) { return offset < 0 ? 0 : offset; }

int normalizedLimit(int limit) { return limit <= 0 ? kDefaultPageSize : limit; }

Book rowToBook(const drogon::orm::Row &row) {
  Book b;
  b.id = row["id"].as<int64_t>();
  b.title = row["title"].as<std::string>();
  b.author = row["author"].as<std::string>();
  b.description =
      row["description"].isNull() ? "" : row["description"].as<std::string>();
  b.coverKey =
      row["cover_key"].isNull() ? "" : row["cover_key"].as<std::string>();
  b.stock = row["stock"].as<int>();
  b.createdAt = row["created_at"].as<std::string>();
  b.updatedAt = row["updated_at"].as<std::string>();
  return b;
}

LoanRecord rowToLoanRecord(const drogon::orm::Row &row) {
  LoanRecord record;
  record.id = row["id"].as<int64_t>();
  record.bookId = row["book_id"].as<int64_t>();
  record.userId = row["user_id"].as<int64_t>();
  record.status = row["status"].as<std::string>();
  record.borrowedAt = row["borrowed_at"].as<std::string>();
  if (!row["returned_at"].isNull()) {
    record.returnedAt = row["returned_at"].as<std::string>();
  }
  record.createdAt = row["created_at"].as<std::string>();
  record.updatedAt = row["updated_at"].as<std::string>();
  return record;
}
} // namespace

void PgClient::init(const std::string &host, int port,
                    const std::string &dbname, const std::string &user,
                    const std::string &password, int connNumber) {
  std::string connStr = "host=" + host + " port=" + std::to_string(port) +
                        " dbname=" + dbname + " user=" + user +
                        " password=" + password;
  g_db = drogon::orm::DbClient::newPgClient(connStr, connNumber);
  LOG_INFO << "PgClient initialized: host=" << host << " port=" << port
           << " dbname=" << dbname << " connNumber=" << connNumber;
}

std::optional<User> PgClient::findUserByName(const std::string &username) {
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, username, password, role, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at "
      "FROM users WHERE username=$1",
      username);
  auto r = f.get();
  if (r.empty())
    return std::nullopt;
  const auto &row = r[0];
  User u;
  u.id = row["id"].as<int64_t>();
  u.username = row["username"].as<std::string>();
  u.password = row["password"].as<std::string>();
  u.role = row["role"].as<std::string>();
  u.createdAt = row["created_at"].as<std::string>();
  u.updatedAt = row["updated_at"].as<std::string>();
  return u;
}

bool PgClient::createUser(const std::string &username,
                          const std::string &passwordHash,
                          const std::string &role) {
  try {
    auto f = g_db->execSqlAsyncFuture(
        "INSERT INTO users (username, password, role) VALUES ($1, $2, $3)",
        username, passwordHash, role);
    f.get();
    return true;
  } catch (...) {
    return false;
  }
}

std::optional<User> PgClient::findUserById(int64_t id) {
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, username, password, role, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at "
      "FROM users WHERE id=$1",
      id);
  auto r = f.get();
  if (r.empty())
    return std::nullopt;
  const auto &row = r[0];
  User u;
  u.id = row["id"].as<int64_t>();
  u.username = row["username"].as<std::string>();
  u.password = row["password"].as<std::string>();
  u.role = row["role"].as<std::string>();
  u.createdAt = row["created_at"].as<std::string>();
  u.updatedAt = row["updated_at"].as<std::string>();
  return u;
}

std::vector<User> PgClient::listUsers(int offset, int limit) {
  offset = normalizedOffset(offset);
  limit = normalizedLimit(limit);
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, username, password, role, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at "
      "FROM users ORDER BY id ASC LIMIT $1 OFFSET $2",
      limit, offset);
  auto r = f.get();
  std::vector<User> out;
  out.reserve(r.size());
  for (const auto &row : r) {
    User u;
    u.id = row["id"].as<int64_t>();
    u.username = row["username"].as<std::string>();
    u.password = row["password"].as<std::string>();
    u.role = row["role"].as<std::string>();
    u.createdAt = row["created_at"].as<std::string>();
    u.updatedAt = row["updated_at"].as<std::string>();
    out.push_back(std::move(u));
  }
  return out;
}

bool PgClient::deleteUser(int64_t id) {
  try {
    auto f = g_db->execSqlAsyncFuture("DELETE FROM users WHERE id=$1", id);
    return f.get().affectedRows() > 0;
  } catch (...) {
    return false;
  }
}

int64_t PgClient::countAdmins() {
  try {
    auto f = g_db->execSqlAsyncFuture(
        "SELECT count(*) AS cnt FROM users WHERE role='admin'");
    auto r = f.get();
    if (r.empty())
      return 0;
    return r[0]["cnt"].as<int64_t>();
  } catch (...) {
    return 0;
  }
}

std::optional<Book> PgClient::findBookById(int64_t id) {
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, title, author, description, cover_key, stock, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at "
      "FROM books WHERE id=$1",
      id);
  auto r = f.get();
  if (r.empty())
    return std::nullopt;
  return rowToBook(r[0]);
}

std::vector<Book> PgClient::listBooks(int offset, int limit) {
  offset = normalizedOffset(offset);
  limit = normalizedLimit(limit);
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, title, author, description, cover_key, stock, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at "
      "FROM books ORDER BY id DESC LIMIT $1 OFFSET $2",
      (int64_t)limit, (int64_t)offset);
  auto r = f.get();
  std::vector<Book> out;
  out.reserve(r.size());
  for (const auto &row : r)
    out.push_back(rowToBook(row));
  return out;
}

int64_t PgClient::createBook(const Book &b) {
  auto f = g_db->execSqlAsyncFuture(
      "INSERT INTO books (title, author, description, cover_key, stock) "
      "VALUES ($1, $2, $3, $4, $5) RETURNING id",
      b.title, b.author, b.description, b.coverKey, (int64_t)b.stock);
  auto r = f.get();
  return r[0]["id"].as<int64_t>();
}

bool PgClient::updateBook(const Book &b) {
  try {
    auto f = g_db->execSqlAsyncFuture(
        "UPDATE books SET title=$1, author=$2, description=$3, "
        "cover_key=$4, stock=$5 WHERE id=$6",
        b.title, b.author, b.description, b.coverKey, (int64_t)b.stock, b.id);
    return f.get().affectedRows() > 0;
  } catch (...) {
    return false;
  }
}

bool PgClient::deleteBook(int64_t id) {
  try {
    auto f = g_db->execSqlAsyncFuture("DELETE FROM books WHERE id=$1", id);
    return f.get().affectedRows() > 0;
  } catch (...) {
    return false;
  }
}

std::optional<LoanRecord> PgClient::findLoanRecordById(int64_t id) {
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, book_id, user_id, status, "
      "to_char(borrowed_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS borrowed_at, "
      "CASE WHEN returned_at IS NULL THEN NULL ELSE "
      "to_char(returned_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') END AS returned_at, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at "
      "FROM loan_records WHERE id=$1",
      id);
  auto r = f.get();
  if (r.empty()) {
    return std::nullopt;
  }
  return rowToLoanRecord(r[0]);
}

std::vector<LoanRecord> PgClient::listLoanRecords(int offset, int limit) {
  offset = normalizedOffset(offset);
  limit = normalizedLimit(limit);
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, book_id, user_id, status, "
      "to_char(borrowed_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS borrowed_at, "
      "CASE WHEN returned_at IS NULL THEN NULL ELSE "
      "to_char(returned_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') END AS returned_at, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at "
      "FROM loan_records ORDER BY id DESC LIMIT $1 OFFSET $2",
      (int64_t)limit, (int64_t)offset);
  auto r = f.get();
  std::vector<LoanRecord> out;
  out.reserve(r.size());
  for (const auto &row : r) {
    out.push_back(rowToLoanRecord(row));
  }
  return out;
}

std::vector<LoanRecord> PgClient::listLoanRecordsByUser(int64_t userId,
                                                        int offset, int limit) {
  offset = normalizedOffset(offset);
  limit = normalizedLimit(limit);
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, book_id, user_id, status, "
      "to_char(borrowed_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS borrowed_at, "
      "CASE WHEN returned_at IS NULL THEN NULL ELSE "
      "to_char(returned_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') END AS returned_at, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at "
      "FROM loan_records WHERE user_id=$1 "
      "ORDER BY id DESC LIMIT $2 OFFSET $3",
      userId, (int64_t)limit, (int64_t)offset);
  auto r = f.get();
  std::vector<LoanRecord> out;
  out.reserve(r.size());
  for (const auto &row : r) {
    out.push_back(rowToLoanRecord(row));
  }
  return out;
}

std::vector<LoanRecord>
PgClient::listActiveLoanRecordsByUser(int64_t userId, int offset, int limit) {
  offset = normalizedOffset(offset);
  limit = normalizedLimit(limit);
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, book_id, user_id, status, "
      "to_char(borrowed_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS borrowed_at, "
      "CASE WHEN returned_at IS NULL THEN NULL ELSE "
      "to_char(returned_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') END AS returned_at, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at "
      "FROM loan_records WHERE user_id=$1 AND status='borrowed' "
      "ORDER BY id DESC LIMIT $2 OFFSET $3",
      userId, (int64_t)limit, (int64_t)offset);
  auto r = f.get();
  std::vector<LoanRecord> out;
  out.reserve(r.size());
  for (const auto &row : r) {
    out.push_back(rowToLoanRecord(row));
  }
  return out;
}

std::optional<int64_t> PgClient::countActiveLoanRecordsByUser(int64_t userId) {
  try {
    auto f =
        g_db->execSqlAsyncFuture("SELECT count(*) AS cnt FROM loan_records "
                                 "WHERE user_id = $1 AND status = 'borrowed'",
                                 userId);
    auto r = f.get();
    if (r.empty())
      return 0;
    return r[0]["cnt"].as<int64_t>();
  } catch (...) {
    return std::nullopt;
  }
}

int64_t PgClient::createLoanRecord(const LoanRecord &record) {
  auto f = g_db->execSqlAsyncFuture(
      "INSERT INTO loan_records "
      "(book_id, user_id, status, borrowed_at, returned_at) "
      "VALUES ($1, $2, $3, $4, $5) RETURNING id",
      record.bookId, record.userId, record.status, record.borrowedAt,
      record.returnedAt);
  auto r = f.get();
  return r[0]["id"].as<int64_t>();
}

bool PgClient::updateLoanRecord(const LoanRecord &record) {
  try {
    auto f = g_db->execSqlAsyncFuture(
        "UPDATE loan_records SET book_id=$1, user_id=$2, status=$3, "
        "borrowed_at=$4, returned_at=$5 WHERE id=$6",
        record.bookId, record.userId, record.status, record.borrowedAt,
        record.returnedAt, record.id);
    return f.get().affectedRows() > 0;
  } catch (...) {
    return false;
  }
}

bool PgClient::deleteLoanRecord(int64_t id) {
  try {
    auto f =
        g_db->execSqlAsyncFuture("DELETE FROM loan_records WHERE id=$1", id);
    return f.get().affectedRows() > 0;
  } catch (...) {
    return false;
  }
}

BorrowBookResult PgClient::borrowBook(int64_t bookId, int64_t userId) {
  try {
    auto trans = g_db->newTransaction();
    auto bookResult = trans->execSqlSync(
        "SELECT stock FROM books WHERE id=$1 FOR UPDATE", bookId);
    if (bookResult.empty()) {
      return {BorrowBookStatus::BookNotFound, 0};
    }

    if (bookResult[0]["stock"].as<int>() <= 0) {
      return {BorrowBookStatus::OutOfStock, 0};
    }

    auto activeLoanResult = trans->execSqlSync(
        "SELECT id FROM loan_records "
        "WHERE book_id=$1 AND user_id=$2 AND status='borrowed' "
        "LIMIT 1",
        bookId, userId);
    if (!activeLoanResult.empty()) {
      return {BorrowBookStatus::AlreadyBorrowed, 0};
    }

    auto updateBookResult =
        trans->execSqlSync("UPDATE books SET stock = stock - 1 "
                           "WHERE id=$1 AND stock > 0 "
                           "RETURNING id",
                           bookId);
    if (updateBookResult.empty()) {
      return {BorrowBookStatus::OutOfStock, 0};
    }

    auto insertLoanResult = trans->execSqlSync(
        "INSERT INTO loan_records (book_id, user_id, status, borrowed_at) "
        "VALUES ($1, $2, 'borrowed', now()) RETURNING id",
        bookId, userId);
    if (insertLoanResult.empty()) {
      trans->rollback();
      return {};
    }

    return {BorrowBookStatus::Borrowed,
            insertLoanResult[0]["id"].as<int64_t>()};
  } catch (...) {
    return {};
  }
}

ReturnBookResult PgClient::returnBook(int64_t bookId, int64_t userId) {
  try {
    auto trans = g_db->newTransaction();
    auto bookResult = trans->execSqlSync(
        "SELECT id FROM books WHERE id=$1 FOR UPDATE", bookId);
    if (bookResult.empty()) {
      return {ReturnBookStatus::BookNotFound, 0};
    }

    auto activeLoanResult = trans->execSqlSync(
        "SELECT id FROM loan_records "
        "WHERE book_id=$1 AND user_id=$2 AND status='borrowed' "
        "ORDER BY id ASC LIMIT 1 FOR UPDATE",
        bookId, userId);
    if (activeLoanResult.empty()) {
      return {ReturnBookStatus::LoanNotFound, 0};
    }

    const auto loanId = activeLoanResult[0]["id"].as<int64_t>();
    auto updateLoanResult =
        trans->execSqlSync("UPDATE loan_records "
                           "SET status='returned', returned_at=now() "
                           "WHERE id=$1 AND status='borrowed' "
                           "RETURNING id",
                           loanId);
    if (updateLoanResult.empty()) {
      trans->rollback();
      return {ReturnBookStatus::LoanNotFound, 0};
    }

    trans->execSqlSync("UPDATE books SET stock = stock + 1 WHERE id=$1",
                       bookId);
    return {ReturnBookStatus::Returned, loanId};
  } catch (...) {
    return {};
  }
}

// 搜索: 基于 pg_trgm 索引命中 title/author/description, 按相似度排序
std::vector<Book> PgClient::search(const std::string &query, int offset,
                                   int limit) {
  if (query.empty()) {
    return {};
  }
  offset = normalizedOffset(offset);
  limit = normalizedLimit(limit);
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, title, author, description, cover_key, stock, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at, "
      "similarity(coalesce(title, '') || ' ' || coalesce(author, '') || ' ' "
      "|| coalesce(description, ''), $1) AS rank "
      "FROM books WHERE (coalesce(title, '') || ' ' || coalesce(author, '') "
      "|| ' ' || coalesce(description, '')) ILIKE '%' || $1 || '%' "
      "ORDER BY rank DESC, id DESC LIMIT $2 OFFSET $3",
      query, limit, offset);
  auto r = f.get();
  std::vector<Book> out;
  out.reserve(r.size());
  for (const auto &row : r)
    out.push_back(rowToBook(row));
  return out;
}

} // namespace libsys
