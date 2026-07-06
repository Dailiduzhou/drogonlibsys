#include "services/AuthService.h"
#include "libsys/utils/PgClient.h"
#include "libsys/utils/RedisClient.h"

#include <crypt.h>
#include <drogon/drogon.h>
#include <optional>

namespace libsys {

namespace {
constexpr const char *kBcryptPrefix = "$2b$";
constexpr unsigned long kBcryptCost = 12;
constexpr size_t kMaxUsernameLength = 64;

bool isBcryptHash(const std::string &hash) {
  return hash.rfind("$2a$", 0) == 0 || hash.rfind("$2b$", 0) == 0 ||
         hash.rfind("$2y$", 0) == 0;
}

std::optional<std::string> hashPassword(const std::string &plain) {
  if (plain.empty() || plain.size() >= CRYPT_MAX_PASSPHRASE_SIZE) {
    return std::nullopt;
  }

  char setting[CRYPT_GENSALT_OUTPUT_SIZE] = {};
  if (!crypt_gensalt_rn(kBcryptPrefix, kBcryptCost, nullptr, 0, setting,
                        sizeof(setting))) {
    return std::nullopt;
  }

  crypt_data data {};
  char *hashed = crypt_rn(plain.c_str(), setting, &data, sizeof(data));
  if (!hashed || hashed[0] == '*') {
    return std::nullopt;
  }
  return std::string(hashed);
}

bool verifyPassword(const std::string &hash, const std::string &plain) {
  if (!isBcryptHash(hash) || plain.empty() ||
      plain.size() >= CRYPT_MAX_PASSPHRASE_SIZE) {
    return false;
  }

  crypt_data data {};
  char *hashed = crypt_rn(plain.c_str(), hash.c_str(), &data, sizeof(data));
  return hashed && hashed[0] != '*' && hash == hashed;
}

bool isValidCredentialInput(const std::string &username,
                            const std::string &password) {
  return !username.empty() && username.size() <= kMaxUsernameLength &&
         !password.empty() && password.size() < CRYPT_MAX_PASSPHRASE_SIZE;
}
} // namespace

void AuthService::initAndStart(const Json::Value &config) {
  (void)config;
}

void AuthService::shutdown() {
}

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

  if (isBcryptHash(user->password)) {
    if (!verifyPassword(user->password, password)) {
      return std::nullopt;
    }
  } else {
    if (user->password != password) {
      return std::nullopt;
    }
    auto passwordHash = hashPassword(password);
    if (passwordHash &&
        !PgClient::updateUserPasswordHash(user->id, *passwordHash)) {
      LOG_WARN << "Failed to upgrade legacy plaintext password for user "
               << user->username;
    }
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
