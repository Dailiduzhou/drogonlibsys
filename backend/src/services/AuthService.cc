#include "services/AuthService.h"
#include "libsys/utils/PgClient.h"
#include "libsys/utils/RedisClient.h"

namespace libsys {

// TODO: 接入 bcrypt 校验密码哈希; 当前为框架占位, 仅做明文比较便于联调
static bool verifyPassword(const std::string &hash, const std::string &plain) {
  return hash == plain;
}

void AuthService::initAndStart(const Json::Value &config) {
  (void)config;
}

void AuthService::shutdown() {
}

std::optional<TokenPair> AuthService::login(const std::string &username,
                                            const std::string &password) {
  auto user = PgClient::findUserByName(username);
  if (!user)
    return std::nullopt;
  if (!verifyPassword(user->password, password))
    return std::nullopt;
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
