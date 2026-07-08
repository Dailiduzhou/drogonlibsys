#include "libsys/utils/RedisClient.h"

#include <hiredis/hiredis.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace libsys {

namespace {
redisContext *g_ctx = nullptr;
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
  auto *r = static_cast<redisReply *>(
      redisCommand(g_ctx, "SET %s 1 EX %d", jti.c_str(), ttlSeconds));
  bool ok = r && r->type != REDIS_REPLY_ERROR;
  if (r)
    freeReplyObject(r);
  return ok;
}

bool RedisClient::isBlacklisted(const std::string &jti) {
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
  auto *r =
      static_cast<redisReply *>(redisCommand(g_ctx, "UNLINK %s", key.c_str()));
  bool ok = r && r->type != REDIS_REPLY_ERROR;
  if (r)
    freeReplyObject(r);
  return ok;
}

bool RedisClient::delByPrefix(const std::string &prefix) {
  std::vector<std::string> keys;
  std::string cursor = "0";
  do {
    auto *r = static_cast<redisReply *>(redisCommand(
        g_ctx, "SCAN %s MATCH %s* COUNT 100", cursor.c_str(), prefix.c_str()));
    if (r == nullptr) {
      return false;
    }
    if (r->type != REDIS_REPLY_ARRAY || r->elements != 2) {
      freeReplyObject(r);
      return false;
    }

    auto *cursorReply = r->element[0];
    auto *keysReply = r->element[1];
    if (!cursorReply || cursorReply->type != REDIS_REPLY_STRING ||
        !keysReply || keysReply->type != REDIS_REPLY_ARRAY) {
      freeReplyObject(r);
      return false;
    }

    cursor.assign(cursorReply->str, cursorReply->len);
    for (size_t i = 0; i < keysReply->elements; ++i) {
      auto *item = keysReply->element[i];
      if (item && item->type == REDIS_REPLY_STRING) {
        keys.emplace_back(item->str, item->len);
      }
    }
    freeReplyObject(r);
  } while (cursor != "0");

  bool ok = true;
  for (const auto &key : keys) {
    ok = del(key) && ok;
  }
  return ok;
}

} // namespace libsys
