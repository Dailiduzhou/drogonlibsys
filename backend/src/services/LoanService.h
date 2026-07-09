#pragma once

#include "libsys/models/LoanRecord.h"
#include <drogon/plugins/Plugin.h>
#include <optional>
#include <vector>

namespace libsys {

// 出借服务:
// 1. 借书/还书负责维护 books.stock 与 loan_records 一致性
// 2. loan_records CRUD 仅用于记录维护
class LoanService : public drogon::Plugin<LoanService> {
public:
  void initAndStart(const Json::Value &config) override;
  void shutdown() override;

  std::optional<LoanRecord> getLoanRecord(int64_t id);
  std::vector<LoanRecord> listLoanRecords(int64_t offset, int64_t limit);
  std::vector<LoanRecord> listLoanRecordsByUser(int64_t userId, int64_t offset,
                                                int64_t limit);
  int64_t createLoanRecord(const LoanRecord &record);
  bool updateLoanRecord(const LoanRecord &record);
  bool deleteLoanRecord(int64_t id);

  BorrowBookResult borrowBook(int64_t bookId, int64_t userId);
  ReturnBookResult returnBook(int64_t bookId, int64_t userId);

  static void invalidateBookStateCaches(int64_t bookId);
};

} // namespace libsys
