#include "filters/JwtAuthFilter.h"
#include "libsys/models/ApiResponse.h"
#include "libsys/utils/JwtUtils.h"
#include "libsys/utils/RedisClient.h"

#include <drogon/HttpAppFramework.h>

namespace libsys {

void JwtAuthFilter::doFilter(const drogon::HttpRequestPtr &req,
                             drogon::FilterCallback &&fcb,
                             drogon::FilterChainCallback &&fccb) {
  auto auth = req->getHeader("Authorization");
  constexpr const char *prefix = "Bearer ";
  if (auth.size() <= sizeof(prefix) - 1 ||
      auth.compare(0, sizeof(prefix) - 1, prefix) != 0) {
    fcb(ApiResponse::fail(401, "missing or invalid Authorization header"));
    return;
  }

  std::string token = auth.substr(sizeof(prefix) - 1);
  JwtClaims claims;
  if (!JwtUtils::verify(token, claims)) {
    fcb(ApiResponse::fail(401, "invalid or expired token"));
    return;
  }

  // JWT 黑名单校验 (登出/刷新后 jti 被 Redis 标记)
  if (RedisClient::isBlacklisted(claims.jti)) {
    fcb(ApiResponse::fail(401, "token revoked"));
    return;
  }

  // 注入用户上下文, 供 Controller 使用
  req->getAttributes()->insert("userId", claims.userId);
  req->getAttributes()->insert("username", claims.username);
  req->getAttributes()->insert("role", claims.role);
  fccb();
}

} // namespace libsys
