#include "controllers/LoanController.h"

#include "libsys/models/ApiResponse.h"
#include "services/LoanService.h"

#include <json/json.h>

namespace libsys {

namespace {
Json::Value loanJson(const LoanRecord &record) {
  Json::Value v;
  v["id"] = (Json::Int64)record.id;
  v["bookId"] = (Json::Int64)record.bookId;
  v["userId"] = (Json::Int64)record.userId;
  v["status"] = record.status;
  v["borrowedAt"] = record.borrowedAt;
  if (record.returnedAt) {
    v["returnedAt"] = *record.returnedAt;
  } else {
    v["returnedAt"] = Json::nullValue;
  }
  v["createdAt"] = record.createdAt;
  v["updatedAt"] = record.updatedAt;
  return v;
}

int64_t currentUserId(const drogon::HttpRequestPtr &req) {
  return req->getAttributes()->get<int64_t>("userId");
}

std::string currentRole(const drogon::HttpRequestPtr &req) {
  return req->getAttributes()->get<std::string>("role");
}

bool isAdmin(const drogon::HttpRequestPtr &req) {
  return currentRole(req) == "admin";
}
} // namespace

void LoanController::list(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  int offset = std::atoi(req->getParameter("offset").c_str());
  int limit = std::atoi(req->getParameter("limit").c_str());
  if (limit <= 0) {
    limit = 20;
  }

  auto *svc = drogon::app().getPlugin<LoanService>();
  const bool admin = isAdmin(req);
  auto records =
      admin ? svc->listLoanRecords(offset, limit)
            : svc->listLoanRecordsByUser(currentUserId(req), offset, limit);
  Json::Value data(Json::arrayValue);
  for (const auto &record : records) {
    data.append(loanJson(record));
  }
  cb(ApiResponse::ok(data));
}

void LoanController::get(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  auto *svc = drogon::app().getPlugin<LoanService>();
  auto record = svc->getLoanRecord(id);
  if (!record) {
    cb(ApiResponse::fail(404, "loan record not found"));
    return;
  }
  if (!isAdmin(req) && record->userId != currentUserId(req)) {
    cb(ApiResponse::fail(403, "forbidden"));
    return;
  }
  cb(ApiResponse::ok(loanJson(*record)));
}

void LoanController::create(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
  if (!isAdmin(req)) {
    cb(ApiResponse::fail(403, "admin only"));
    return;
  }

  auto json = req->getJsonObject();
  if (!json) {
    cb(ApiResponse::fail(400, "json body required"));
    return;
  }

  LoanRecord record;
  record.bookId = json->get("bookId", 0).asInt64();
  record.userId = json->get("userId", 0).asInt64();
  record.status = json->get("status", "borrowed").asString();
  record.borrowedAt = json->get("borrowedAt", "").asString();
  if (json->isMember("returnedAt") && !(*json)["returnedAt"].isNull()) {
    record.returnedAt = (*json)["returnedAt"].asString();
  }
  if (record.bookId <= 0 || record.userId <= 0 || record.borrowedAt.empty()) {
    cb(ApiResponse::fail(400, "bookId, userId and borrowedAt required"));
    return;
  }

  auto *svc = drogon::app().getPlugin<LoanService>();
  const auto id = svc->createLoanRecord(record);
  Json::Value data;
  data["id"] = (Json::Int64)id;
  cb(ApiResponse::ok(data));
}

void LoanController::update(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  if (!isAdmin(req)) {
    cb(ApiResponse::fail(403, "admin only"));
    return;
  }

  auto json = req->getJsonObject();
  if (!json) {
    cb(ApiResponse::fail(400, "json body required"));
    return;
  }

  LoanRecord record;
  record.id = id;
  record.bookId = json->get("bookId", 0).asInt64();
  record.userId = json->get("userId", 0).asInt64();
  record.status = json->get("status", "").asString();
  record.borrowedAt = json->get("borrowedAt", "").asString();
  if (json->isMember("returnedAt") && !(*json)["returnedAt"].isNull()) {
    record.returnedAt = (*json)["returnedAt"].asString();
  }
  if (record.bookId <= 0 || record.userId <= 0 || record.status.empty() ||
      record.borrowedAt.empty()) {
    cb(ApiResponse::fail(400,
                         "bookId, userId, status and borrowedAt required"));
    return;
  }

  auto *svc = drogon::app().getPlugin<LoanService>();
  if (!svc->updateLoanRecord(record)) {
    cb(ApiResponse::fail(500, "update failed"));
    return;
  }
  cb(ApiResponse::ok());
}

void LoanController::remove(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t id) {
  if (!isAdmin(req)) {
    cb(ApiResponse::fail(403, "admin only"));
    return;
  }

  auto *svc = drogon::app().getPlugin<LoanService>();
  if (!svc->deleteLoanRecord(id)) {
    cb(ApiResponse::fail(500, "delete failed"));
    return;
  }
  cb(ApiResponse::ok());
}

void LoanController::borrow(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t bookId) {
  auto *svc = drogon::app().getPlugin<LoanService>();
  const auto result = svc->borrowBook(bookId, currentUserId(req));

  switch (result.status) {
  case BorrowBookStatus::Borrowed: {
    auto record = svc->getLoanRecord(result.loanId);
    if (record) {
      cb(ApiResponse::ok(loanJson(*record)));
      return;
    }
    Json::Value data;
    data["id"] = (Json::Int64)result.loanId;
    cb(ApiResponse::ok(data));
    return;
  }
  case BorrowBookStatus::BookNotFound:
    cb(ApiResponse::fail(404, "book not found"));
    return;
  case BorrowBookStatus::OutOfStock:
    cb(ApiResponse::fail(409, "book out of stock"));
    return;
  case BorrowBookStatus::AlreadyBorrowed:
    cb(ApiResponse::fail(409, "book already borrowed by current user"));
    return;
  case BorrowBookStatus::Error:
  default:
    cb(ApiResponse::fail(500, "borrow failed"));
    return;
  }
}

void LoanController::returnBook(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&cb, int64_t bookId) {
  auto *svc = drogon::app().getPlugin<LoanService>();
  const auto result = svc->returnBook(bookId, currentUserId(req));

  switch (result.status) {
  case ReturnBookStatus::Returned: {
    auto record = svc->getLoanRecord(result.loanId);
    if (record) {
      cb(ApiResponse::ok(loanJson(*record)));
      return;
    }
    Json::Value data;
    data["id"] = (Json::Int64)result.loanId;
    cb(ApiResponse::ok(data));
    return;
  }
  case ReturnBookStatus::BookNotFound:
    cb(ApiResponse::fail(404, "book not found"));
    return;
  case ReturnBookStatus::LoanNotFound:
    cb(ApiResponse::fail(404, "active loan record not found"));
    return;
  case ReturnBookStatus::Error:
  default:
    cb(ApiResponse::fail(500, "return failed"));
    return;
  }
}

} // namespace libsys
