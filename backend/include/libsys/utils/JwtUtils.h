#pragma once

#include <string>

namespace libsys {

// JWT 双 Token 机制
//  - access  token: 短有效期, 用于接口鉴权
//  - refresh token: 长有效期, 用于换取新的 token 对
struct TokenPair {
  std::string accessToken;
  std::string refreshToken;
  std::string jti; // JWT ID, 用于黑名单
  int64_t accessExpiresAt{0};
  int64_t refreshExpiresAt{0};
};

// JWT 载荷
struct JwtClaims {
  int64_t userId{0};
  std::string username;
  std::string role;
  std::string jti;
};

class JwtUtils {
public:
  static void init(const std::string &secret, const std::string &issuer,
                   int accessTtl, int refreshTtl);

  // 签发 access + refresh token 对
  static TokenPair issue(int64_t userId, const std::string &username,
                         const std::string &role);

  // 校验并解析 token; 失败返回 false
  static bool verify(const std::string &token, JwtClaims &out);

  // 计算指定 token 的剩余有效期 (秒), 用于黑名单 TTL
  static int remainingTtl(const std::string &token);
};

} // namespace libsys
