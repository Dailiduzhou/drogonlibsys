#include "services/UserService.h"

#include "libsys/utils/PgClient.h"
#include <cstdint>

namespace libsys {

void UserService::initAndStart(const Json::Value &config) { (void)config; }

void UserService::shutdown() {}

std::optional<User> UserService::getUser(int64_t id) {
  return PgClient::findUserById(id);
}

std::vector<User> UserService::listUsers(int64_t offset, int64_t limit) {
  return PgClient::listUsers(offset, limit);
}

bool UserService::deleteUser(int64_t id) { return PgClient::deleteUser(id); }

int64_t UserService::countAdmins() { return PgClient::countAdmins(); }

std::optional<int64_t>
UserService::countActiveLoansByUser(int64_t userId) {
  return PgClient::countActiveLoanRecordsByUser(userId);
}

} // namespace libsys
