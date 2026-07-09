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

struct PostgresConfig {
  std::string host;
  int port{5432};
  std::string dbname;
  std::string user;
  std::string password;
  int connNumber{8};
};

struct MinioConfig {
  std::string endpoint;
  std::string publicEndpoint;
  std::string accessKey;
  std::string secretKey;
  std::string bucket;
  std::string region{"us-east-1"};
  bool secure{false};
  bool publicSecure{false};
};

struct AppConfig {
  std::string filePath;
  JwtConfig jwt;
  RedisConfig redis;
  PostgresConfig postgres;
  MinioConfig minio;
};

std::string resolveConfigPath();
AppConfig loadAppConfigFromCurrentApp();
AppConfig loadAppConfigFromCustomConfig(const Json::Value &root);
AppConfig loadAppConfig();

} // namespace libsys
