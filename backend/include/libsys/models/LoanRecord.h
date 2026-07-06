#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace libsys {

// 图书出借记录
struct LoanRecord {
  int64_t id{0};
  int64_t bookId{0};
  int64_t userId{0};
  std::string status; // borrowed / returned
  std::string borrowedAt;
  std::optional<std::string> returnedAt;
  std::string createdAt;
  std::string updatedAt;
};

enum class BorrowBookStatus {
  Borrowed,
  BookNotFound,
  OutOfStock,
  AlreadyBorrowed,
  Error,
};

struct BorrowBookResult {
  BorrowBookStatus status{BorrowBookStatus::Error};
  int64_t loanId{0};
};

enum class ReturnBookStatus {
  Returned,
  BookNotFound,
  LoanNotFound,
  Error,
};

struct ReturnBookResult {
  ReturnBookStatus status{ReturnBookStatus::Error};
  int64_t loanId{0};
};

} // namespace libsys
