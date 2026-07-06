#include "libsys/utils/JwtUtils.h"

#include <chrono>
#include <iomanip>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/open-source-parsers-jsoncpp/defaults.h>
#include <random>
#include <sstream>

namespace libsys {

namespace {
std::string g_secret;
std::string g_issuer;
int g_accessTtl = 900;     // 15 分钟
int g_refreshTtl = 604800; // 7 天

std::string genJti() {
  std::random_device rd;
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (int i = 0; i < 4; ++i) {
    ss << std::setw(8) << rd();
  }
  return ss.str();
}
} // namespace

void JwtUtils::init(const std::string &secret, const std::string &issuer,
                    int accessTtl, int refreshTtl) {
  g_secret = secret;
  g_issuer = issuer;
  g_accessTtl = accessTtl;
  g_refreshTtl = refreshTtl;
}

TokenPair JwtUtils::issue(int64_t userId, const std::string &username,
                          const std::string &role) {
  TokenPair pair;
  pair.jti = genJti();
  auto now = std::chrono::system_clock::now();

  auto build = [&](int ttl) {
    return jwt::create()
        .set_issuer(g_issuer)
        .set_type("JWT")
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::seconds(ttl))
        .set_id(pair.jti)
        .set_payload_claim("uid", jwt::claim(std::to_string(userId)))
        .set_payload_claim("name", jwt::claim(username))
        .set_payload_claim("role", jwt::claim(role))
        .sign(jwt::algorithm::hs256{g_secret});
  };

  pair.accessToken = build(g_accessTtl);
  pair.refreshToken = build(g_refreshTtl);
  pair.accessExpiresAt =
      std::chrono::duration_cast<std::chrono::seconds>(
          (now + std::chrono::seconds(g_accessTtl)).time_since_epoch())
          .count();
  pair.refreshExpiresAt =
      std::chrono::duration_cast<std::chrono::seconds>(
          (now + std::chrono::seconds(g_refreshTtl)).time_since_epoch())
          .count();
  return pair;
}

bool JwtUtils::verify(const std::string &token, JwtClaims &out) {
  try {
    auto decoded = jwt::decode(token);
    auto verifier = jwt::verify()
                        .allow_algorithm(jwt::algorithm::hs256{g_secret})
                        .with_issuer(g_issuer);
    verifier.verify(decoded);

    out.jti = decoded.get_id();
    out.userId = std::stoll(decoded.get_payload_claim("uid").as_string());
    out.username = decoded.get_payload_claim("name").as_string();
    out.role = decoded.get_payload_claim("role").as_string();
    return true;
  } catch (...) {
    return false;
  }
}

int JwtUtils::remainingTtl(const std::string &token) {
  try {
    auto decoded = jwt::decode(token);
    auto exp = decoded.get_expires_at();
    auto now = std::chrono::system_clock::now();
    auto secs =
        std::chrono::duration_cast<std::chrono::seconds>(exp - now).count();
    return secs > 0 ? static_cast<int>(secs) : 0;
  } catch (...) {
    return 0;
  }
}

} // namespace libsys
