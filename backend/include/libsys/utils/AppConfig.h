#pragma once

#include <json/value.h>
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
  std::string region{"us-east-1"};
  bool secure{false};
};

struct AppConfig {
  std::string filePath;
  JwtConfig jwt;
  RedisConfig redis;
  MinioConfig minio;
};

std::string resolveConfigPath();
AppConfig loadAppConfigFromCurrentApp();
AppConfig loadAppConfigFromCustomConfig(const Json::Value &root);
AppConfig loadAppConfig();

} // namespace libsys
