#include "libsys/utils/RedisClient.h"

#include <hiredis/hiredis.h>
#include <stdexcept>

namespace libsys {

namespace {
redisContext *g_ctx = nullptr;

void ensure() {
  if (!g_ctx) {
    throw std::runtime_error("RedisClient not initialized");
  }
}
} // namespace

void RedisClient::init(const std::string &host, int port,
                       const std::string &password) {
  if (g_ctx) {
    redisFree(g_ctx);
    g_ctx = nullptr;
  }
  g_ctx = redisConnect(host.c_str(), port);
  if (g_ctx == nullptr || g_ctx->err) {
    throw std::runtime_error(std::string("Redis connect failed: ") +
                             (g_ctx ? g_ctx->errstr : "alloc error"));
  }
  if (!password.empty()) {
    auto *r = static_cast<redisReply *>(
        redisCommand(g_ctx, "AUTH %s", password.c_str()));
    if (r) {
      freeReplyObject(r);
    }
  }
}

bool RedisClient::blacklistAdd(const std::string &jti, int ttlSeconds) {
  ensure();
  auto *r = static_cast<redisReply *>(
      redisCommand(g_ctx, "SET %s 1 EX %d", jti.c_str(), ttlSeconds));
  bool ok = r && r->type != REDIS_REPLY_ERROR;
  if (r)
    freeReplyObject(r);
  return ok;
}

bool RedisClient::isBlacklisted(const std::string &jti) {
  ensure();
  auto *r =
      static_cast<redisReply *>(redisCommand(g_ctx, "EXISTS %s", jti.c_str()));
  bool blacklisted = false;
  if (r && r->type == REDIS_REPLY_INTEGER) {
    blacklisted = r->integer != 0;
  }
  if (r)
    freeReplyObject(r);
  return blacklisted;
}

bool RedisClient::set(const std::string &key, const std::string &value,
                      int ttlSeconds) {
  ensure();
  redisReply *r;
  if (ttlSeconds > 0) {
    r = static_cast<redisReply *>(redisCommand(
        g_ctx, "SET %s %s EX %d", key.c_str(), value.c_str(), ttlSeconds));
  } else {
    r = static_cast<redisReply *>(
        redisCommand(g_ctx, "SET %s %s", key.c_str(), value.c_str()));
  }
  bool ok = r && r->type != REDIS_REPLY_ERROR;
  if (r)
    freeReplyObject(r);
  return ok;
}

std::optional<std::string> RedisClient::get(const std::string &key) {
  ensure();
  auto *r =
      static_cast<redisReply *>(redisCommand(g_ctx, "GET %s", key.c_str()));
  std::optional<std::string> out;
  if (r && r->type == REDIS_REPLY_STRING) {
    out = std::string(r->str, r->len);
  }
  if (r)
    freeReplyObject(r);
  return out;
}

bool RedisClient::del(const std::string &key) {
  ensure();
  auto *r =
      static_cast<redisReply *>(redisCommand(g_ctx, "DEL %s", key.c_str()));
  bool ok = r && r->type != REDIS_REPLY_ERROR;
  if (r)
    freeReplyObject(r);
  return ok;
}

} // namespace libsys
