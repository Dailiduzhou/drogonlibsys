#include "services/AuthService.h"
#include "libsys/utils/PgClient.h"
#include "libsys/utils/RedisClient.h"

#include <libsys_c/password_hash.h>

#include <cstdlib>
#include <exception>
#include <memory>
#include <optional>
#include <string>

namespace libsys {

namespace {
constexpr size_t kMaxUsernameLength = 64;
constexpr size_t kMaxPasswordLength = 512;

bool isValidCredentialInput(const std::string &username,
                            const std::string &password) {
  return !username.empty() && username.size() <= kMaxUsernameLength &&
         !password.empty() && password.size() < kMaxPasswordLength;
}

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

bool isUniqueViolation(const std::exception &e) {
  const std::string what = e.what();
  return what.find("23505") != std::string::npos ||
         what.find("unique") != std::string::npos;
}
} // namespace

void AuthService::initAndStart(const Json::Value &config) { (void)config; }

void AuthService::shutdown() {}

RegisterResult AuthService::registerUser(const std::string &username,
                                         const std::string &password) {
  if (!isValidCredentialInput(username, password)) {
    return {RegisterStatus::InvalidInput, std::nullopt};
  }

  auto passwordHash = hashPassword(password);
  if (!passwordHash) {
    return {RegisterStatus::Failed, std::nullopt};
  }

  try {
    PgClient::createUser(username, *passwordHash, "user");
  } catch (const std::exception &e) {
    if (isUniqueViolation(e)) {
      return {RegisterStatus::UsernameTaken, std::nullopt};
    }
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
  if (!JwtUtils::verify(refreshToken, claims, "refresh"))
    return std::nullopt;
  if (RedisClient::isBlacklisted(claims.jti))
    return std::nullopt;

  int ttl = JwtUtils::remainingTtl(refreshToken);
  if (!RedisClient::blacklistAdd(claims.jti, ttl))
    return std::nullopt;

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
