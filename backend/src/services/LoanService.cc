#include "services/LoanService.h"

#include "libsys/utils/PgClient.h"
#include "libsys/utils/RedisClient.h"
#include "services/BookService.h"

namespace libsys {

namespace {
bool invalidateSearchCaches() { return RedisClient::delByPrefix("search:"); }
} // namespace

void LoanService::initAndStart(const Json::Value &config) { (void)config; }

void LoanService::shutdown() {}

std::optional<LoanRecord> LoanService::getLoanRecord(int64_t id) {
  return PgClient::findLoanRecordById(id);
}

std::vector<LoanRecord> LoanService::listLoanRecords(int offset, int limit) {
  return PgClient::listLoanRecords(offset, limit);
}

std::vector<LoanRecord>
LoanService::listLoanRecordsByUser(int64_t userId, int offset, int limit) {
  return PgClient::listLoanRecordsByUser(userId, offset, limit);
}

int64_t LoanService::createLoanRecord(const LoanRecord &record) {
  return PgClient::createLoanRecord(record);
}

bool LoanService::updateLoanRecord(const LoanRecord &record) {
  return PgClient::updateLoanRecord(record);
}

bool LoanService::deleteLoanRecord(int64_t id) {
  return PgClient::deleteLoanRecord(id);
}

BorrowBookResult LoanService::borrowBook(int64_t bookId, int64_t userId) {
  auto result = PgClient::borrowBook(bookId, userId);
  if (result.status == BorrowBookStatus::Borrowed) {
    invalidateBookStateCaches(bookId);
  }
  return result;
}

ReturnBookResult LoanService::returnBook(int64_t bookId, int64_t userId) {
  auto result = PgClient::returnBook(bookId, userId);
  if (result.status == ReturnBookStatus::Returned) {
    invalidateBookStateCaches(bookId);
  }
  return result;
}

void LoanService::invalidateBookStateCaches(int64_t bookId) {
  RedisClient::del(BookService::bookKey(bookId));
  invalidateSearchCaches();
}

} // namespace libsys
