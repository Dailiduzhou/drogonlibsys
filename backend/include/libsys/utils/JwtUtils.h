#pragma once

#include <cstdint>
#include <string>

namespace libsys {

// JWT 双 Token 机制
//  - access  token: 短有效期, 用于接口鉴权
//  - refresh token: 长有效期, 用于换取新的 token 对
struct TokenPair {
  std::string accessToken;
  std::string refreshToken;
  std::string accessJti;
  std::string refreshJti;
  int64_t accessExpiresAt{0};
  int64_t refreshExpiresAt{0};
  // 用户上下文 (同步下发, 便于前端做 UI 门禁; 真伪仍以 token 为准)
  int64_t userId{0};
  std::string username;
  std::string role;
};

// JWT 载荷
struct JwtClaims {
  int64_t userId{0};
  std::string username;
  std::string role;
  std::string jti;
  std::string tokenType;
};

class JwtUtils {
public:
  static void init(const std::string &secret, const std::string &issuer,
                   int accessTtl, int refreshTtl);

  // 签发 access + refresh token 对
  static TokenPair issue(int64_t userId, const std::string &username,
                         const std::string &role);

  // 校验并解析 token; 失败返回 false
  static bool verify(const std::string &token, JwtClaims &out,
                     const std::string &expectedType = "");

  // 计算指定 token 的剩余有效期 (秒), 用于黑名单 TTL
  static int remainingTtl(const std::string &token);
};

} // namespace libsys
