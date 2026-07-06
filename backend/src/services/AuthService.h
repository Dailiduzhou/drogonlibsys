#pragma once

#include "libsys/utils/JwtUtils.h"
#include <drogon/plugins/Plugin.h>
#include <optional>
#include <string>

namespace libsys {

enum class RegisterStatus {
  Success,
  InvalidInput,
  UsernameTaken,
  Failed,
};

struct RegisterResult {
  RegisterStatus status{RegisterStatus::Failed};
  std::optional<TokenPair> tokens;
};

// 鉴权服务: 登录 / 刷新 / 登出
// 参见 AGENTS.md 2.1 双 Token 机制 + JWT 黑名单
class AuthService : public drogon::Plugin<AuthService> {
public:
  void initAndStart(const Json::Value &config) override;
  void shutdown() override;

  // 注册: 创建普通用户并返回双 Token
  RegisterResult registerUser(const std::string &username,
                              const std::string &password);

  // 登录: 校验用户名密码, 成功返回双 Token
  std::optional<TokenPair> login(const std::string &username,
                                 const std::string &password);

  // 刷新: 校验 refresh token, 将旧 jti 加入黑名单, 下发新 Token 对
  std::optional<TokenPair> refresh(const std::string &refreshToken);

  // 登出: 将 token 的 jti 加入 Redis 黑名单 (TTL = 剩余有效期)
  bool logout(const std::string &token);
};

} // namespace libsys
