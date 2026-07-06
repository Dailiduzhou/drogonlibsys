#pragma once

#include <drogon/HttpFilter.h>

namespace libsys {

// JWT 鉴权中间件 (Drogon HttpFilter)
// 请求链路: 解析 Authorization Bearer Token -> 校验签名 ->
//           查 Redis 黑名单 -> 通过则注入用户上下文, 否则 401
// 参见 AGENTS.md 2.1 中间件拦截 + JWT 黑名单
class JwtAuthFilter : public drogon::HttpFilter<JwtAuthFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback &&fcb,
                  drogon::FilterChainCallback &&fccb) override;
};

}  // namespace libsys
