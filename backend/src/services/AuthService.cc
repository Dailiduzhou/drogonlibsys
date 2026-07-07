#include "services/AuthService.h"
#include "libsys/utils/PgClient.h"
#include "libsys/utils/RedisClient.h"

#include <libsys_c/password_hash.h>

#include <cstdlib>
#include <optional>
#include <string>

namespace libsys {

namespace {
constexpr size_t kMaxUsernameLength = 64;
constexpr size_t kMaxPasswordLength =
    512; /* 与 CRYPT_MAX_PASSPHRASE_SIZE 一致 */

bool isValidCredentialInput(const std::string &username,
                            const std::string &password) {
  return !username.empty() && username.size() <= kMaxUsernameLength &&
         !password.empty() && password.size() < kMaxPasswordLength;
}

/* RAII 包装 C 接口返回的 malloc 字符串 */
struct CStringDeleter {
  void operator()(char *p) const { std::free(p); }
};
using CStringPtr = std::unique_ptr<char, CStringDeleter>;

std::optional<std::string> hashPassword(const std::string &plain) {
  CStringPtr raw(password_hash(plain.c_str()));
  if (!raw) {
    return std::nullopt;
  }
  return std::string(raw.get());
}

bool verifyPassword(const std::string &hash, const std::string &plain) {
  return password_verify(hash.c_str(), plain.c_str()) == 1;
}
} // namespace

void AuthService::initAndStart(const Json::Value &config) { (void)config; }

void AuthService::shutdown() {}

RegisterResult AuthService::registerUser(const std::string &username,
                                         const std::string &password) {
  if (!isValidCredentialInput(username, password)) {
    return {RegisterStatus::InvalidInput, std::nullopt};
  }

  if (PgClient::findUserByName(username)) {
    return {RegisterStatus::UsernameTaken, std::nullopt};
  }

  auto passwordHash = hashPassword(password);
  if (!passwordHash) {
    return {RegisterStatus::Failed, std::nullopt};
  }

  if (!PgClient::createUser(username, *passwordHash, "user")) {
    return {RegisterStatus::Failed, std::nullopt};
  }

  auto user = PgClient::findUserByName(username);
  if (!user) {
    return {RegisterStatus::Failed, std::nullopt};
  }

  return {RegisterStatus::Success,
          JwtUtils::issue(user->id, user->username, user->role)};
}

std::optional<TokenPair> AuthService::login(const std::string &username,
                                            const std::string &password) {
  auto user = PgClient::findUserByName(username);
  if (!user)
    return std::nullopt;

  if (!verifyPassword(user->password, password)) {
    return std::nullopt;
  }

  return JwtUtils::issue(user->id, user->username, user->role);
}

std::optional<TokenPair> AuthService::refresh(const std::string &refreshToken) {
  JwtClaims claims;
  if (!JwtUtils::verify(refreshToken, claims))
    return std::nullopt;
  if (RedisClient::isBlacklisted(claims.jti))
    return std::nullopt;

  // 旧 refresh token 加入黑名单, 防止重放
  int ttl = JwtUtils::remainingTtl(refreshToken);
  RedisClient::blacklistAdd(claims.jti, ttl);

  return JwtUtils::issue(claims.userId, claims.username, claims.role);
}

bool AuthService::logout(const std::string &token) {
  JwtClaims claims;
  if (!JwtUtils::verify(token, claims))
    return false;
  int ttl = JwtUtils::remainingTtl(token);
  return RedisClient::blacklistAdd(claims.jti, ttl);
}

} // namespace libsys
