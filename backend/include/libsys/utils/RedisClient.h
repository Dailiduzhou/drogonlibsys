#pragma once

#include <optional>
#include <string>

namespace libsys {

// Redis 客户端封装
// 职责:
//  1. JWT 黑名单 (jti -> 1, 带 TTL)
//  2. 图书元数据 / 搜索结果缓存
class RedisClient {
public:
  static void init(const std::string &host, int port,
                   const std::string &password = "");

  // ---- JWT 黑名单 ----
  // 将 jti 加入黑名单, TTL = 剩余有效期
  static bool blacklistAdd(const std::string &jti, int ttlSeconds);
  static bool isBlacklisted(const std::string &jti);

  // ---- 通用缓存 ----
  static bool set(const std::string &key, const std::string &value,
                  int ttlSeconds = 0);
  static std::optional<std::string> get(const std::string &key);
  static bool del(const std::string &key);
};

} // namespace libsys
