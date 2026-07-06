#include "libsys/utils/PgClient.h"

#include <drogon/drogon.h>
#include <drogon/orm/DbClient.h>

namespace libsys {

namespace {
drogon::orm::DbClientPtr g_db;

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

std::string str(const std::string &s) { return s; }
} // namespace

void PgClient::init() { g_db = drogon::app().getDbClient("pg"); }

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
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, title, author, description, cover_key, stock, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at "
      "FROM books ORDER BY id DESC LIMIT $1 OFFSET $2",
      limit, offset);
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
      b.title, b.author, b.description, b.coverKey, b.stock);
  auto r = f.get();
  return r[0]["id"].as<int64_t>();
}

bool PgClient::updateBook(const Book &b) {
  try {
    auto f = g_db->execSqlAsyncFuture(
        "UPDATE books SET title=$1, author=$2, description=$3, "
        "cover_key=$4, stock=$5 WHERE id=$6",
        b.title, b.author, b.description, b.coverKey, b.stock, b.id);
    f.get();
    return true;
  } catch (...) {
    return false;
  }
}

bool PgClient::deleteBook(int64_t id) {
  try {
    auto f = g_db->execSqlAsyncFuture("DELETE FROM books WHERE id=$1", id);
    f.get();
    return true;
  } catch (...) {
    return false;
  }
}

// 全文检索: 命中 books.tsv GIN 倒排索引, 按 ts_rank 权重排序
std::vector<Book> PgClient::search(const std::string &query, int offset,
                                   int limit) {
  auto f = g_db->execSqlAsyncFuture(
      "SELECT id, title, author, description, cover_key, stock, "
      "to_char(created_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS created_at, "
      "to_char(updated_at, 'YYYY-MM-DD\"T\"HH24:MI:SS') AS updated_at, "
      "ts_rank(tsv, plainto_tsquery('simple', $1)) AS rank "
      "FROM books WHERE tsv @@ plainto_tsquery('simple', $1) "
      "ORDER BY rank DESC LIMIT $2 OFFSET $3",
      query, limit, offset);
  auto r = f.get();
  std::vector<Book> out;
  out.reserve(r.size());
  for (const auto &row : r)
    out.push_back(rowToBook(row));
  return out;
}

} // namespace libsys
