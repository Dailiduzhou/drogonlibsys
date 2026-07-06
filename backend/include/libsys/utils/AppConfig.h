#pragma once

#include <string>

namespace libsys {

struct JwtConfig {
  std::string secret;
  std::string issuer;
  int accessTtlSeconds{0};
  int refreshTtlSeconds{0};
};

struct RedisConfig {
  std::string host;
  int port{0};
  std::string password;
};

struct MinioConfig {
  std::string endpoint;
  std::string accessKey;
  std::string secretKey;
  std::string bucket;
  bool secure{false};
};

struct AppConfig {
  std::string filePath;
  JwtConfig jwt;
  RedisConfig redis;
  MinioConfig minio;
};

std::string resolveConfigPath();
AppConfig loadAppConfig();

} // namespace libsys
