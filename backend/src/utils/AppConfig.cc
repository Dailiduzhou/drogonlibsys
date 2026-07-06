#include "libsys/utils/AppConfig.h"

#include <drogon/drogon.h>

#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace libsys {

namespace {

std::string valuePath(std::string_view path) {
  return std::string("custom_config.") + std::string(path);
}

std::string joinPath(std::string_view parentPath, std::string_view key) {
  if (parentPath.empty()) {
    return std::string(key);
  }
  return std::string(parentPath) + "." + std::string(key);
}

const Json::Value &requireMember(const Json::Value &parent,
                                 std::string_view parentPath,
                                 std::string_view key) {
  const auto keyString = std::string(key);
  if (!parent.isMember(keyString)) {
    throw std::runtime_error("missing required config: " +
                             valuePath(joinPath(parentPath, key)));
  }
  return parent[keyString];
}

const Json::Value &requireObject(const Json::Value &parent,
                                 std::string_view parentPath,
                                 std::string_view key) {
  const auto &value = requireMember(parent, parentPath, key);
  if (!value.isObject()) {
    throw std::runtime_error(valuePath(joinPath(parentPath, key)) +
                             " must be an object");
  }
  return value;
}

std::string requireString(const Json::Value &parent,
                          std::string_view parentPath, std::string_view key) {
  const auto &value = requireMember(parent, parentPath, key);
  if (!value.isString()) {
    throw std::runtime_error(valuePath(joinPath(parentPath, key)) +
                             " must be a string");
  }
  const auto out = value.asString();
  if (out.empty()) {
    throw std::runtime_error(valuePath(joinPath(parentPath, key)) +
                             " must not be empty");
  }
  return out;
}

int requirePositiveInt(const Json::Value &parent, std::string_view parentPath,
                       std::string_view key) {
  const auto &value = requireMember(parent, parentPath, key);
  if (!value.isInt()) {
    throw std::runtime_error(valuePath(joinPath(parentPath, key)) +
                             " must be an integer");
  }
  const int out = value.asInt();
  if (out <= 0) {
    throw std::runtime_error(valuePath(joinPath(parentPath, key)) +
                             " must be greater than 0");
  }
  return out;
}

bool requireBool(const Json::Value &parent, std::string_view parentPath,
                 std::string_view key) {
  const auto &value = requireMember(parent, parentPath, key);
  if (!value.isBool()) {
    throw std::runtime_error(valuePath(joinPath(parentPath, key)) +
                             " must be a boolean");
  }
  return value.asBool();
}

int requirePort(const Json::Value &parent, std::string_view parentPath,
                std::string_view key) {
  const int port = requirePositiveInt(parent, parentPath, key);
  if (port > 65535) {
    throw std::runtime_error(valuePath(joinPath(parentPath, key)) +
                             " must be in range 1-65535");
  }
  return port;
}

const char *readEnv(const char *name) {
  const char *value = std::getenv(name);
  if (value == nullptr || *value == '\0') {
    return nullptr;
  }
  return value;
}

int parsePositiveIntEnv(const char *name) {
  const char *value = readEnv(name);
  if (value == nullptr) {
    return -1;
  }
  try {
    const int parsed = std::stoi(value);
    if (parsed <= 0) {
      throw std::runtime_error(std::string(name) + " must be greater than 0");
    }
    return parsed;
  } catch (const std::invalid_argument &) {
    throw std::runtime_error(std::string(name) + " must be a valid integer");
  } catch (const std::out_of_range &) {
    throw std::runtime_error(std::string(name) + " is out of range");
  }
}

bool parseBoolEnv(const char *name, bool currentValue) {
  const char *value = readEnv(name);
  if (value == nullptr) {
    return currentValue;
  }
  const std::string text(value);
  if (text == "1" || text == "true" || text == "TRUE" || text == "yes" ||
      text == "on") {
    return true;
  }
  if (text == "0" || text == "false" || text == "FALSE" || text == "no" ||
      text == "off") {
    return false;
  }
  throw std::runtime_error(std::string(name) +
                           " must be one of: true/false/1/0/yes/no/on/off");
}

void applyEnvOverrides(AppConfig &config) {
  if (const char *value = readEnv("LIBSYS_JWT_SECRET"); value != nullptr) {
    config.jwt.secret = value;
  }
  if (const char *value = readEnv("LIBSYS_JWT_ISSUER"); value != nullptr) {
    config.jwt.issuer = value;
  }
  if (const int value = parsePositiveIntEnv("LIBSYS_JWT_ACCESS_TTL_SECONDS");
      value > 0) {
    config.jwt.accessTtlSeconds = value;
  }
  if (const int value = parsePositiveIntEnv("LIBSYS_JWT_REFRESH_TTL_SECONDS");
      value > 0) {
    config.jwt.refreshTtlSeconds = value;
  }

  if (const char *value = readEnv("LIBSYS_REDIS_HOST"); value != nullptr) {
    config.redis.host = value;
  }
  if (const int value = parsePositiveIntEnv("LIBSYS_REDIS_PORT"); value > 0) {
    if (value > 65535) {
      throw std::runtime_error("LIBSYS_REDIS_PORT must be in range 1-65535");
    }
    config.redis.port = value;
  }
  if (const char *value = readEnv("LIBSYS_REDIS_PASSWORD"); value != nullptr) {
    config.redis.password = value;
  }

  if (const char *value = readEnv("LIBSYS_MINIO_ENDPOINT"); value != nullptr) {
    config.minio.endpoint = value;
  }
  if (const char *value = readEnv("LIBSYS_MINIO_ACCESS_KEY");
      value != nullptr) {
    config.minio.accessKey = value;
  }
  if (const char *value = readEnv("LIBSYS_MINIO_SECRET_KEY");
      value != nullptr) {
    config.minio.secretKey = value;
  }
  if (const char *value = readEnv("LIBSYS_MINIO_BUCKET"); value != nullptr) {
    config.minio.bucket = value;
  }
  config.minio.secure =
      parseBoolEnv("LIBSYS_MINIO_SECURE", config.minio.secure);
}

void validateConfig(const AppConfig &config) {
  if (config.jwt.refreshTtlSeconds <= config.jwt.accessTtlSeconds) {
    throw std::runtime_error(
        "custom_config.jwt.refresh_ttl_seconds must be greater than "
        "custom_config.jwt.access_ttl_seconds");
  }
}

} // namespace

std::string resolveConfigPath() {
  if (const char *configPath = readEnv("LIBSYS_CONFIG_PATH");
      configPath != nullptr) {
    return configPath;
  }
  return "config/config.json";
}

AppConfig loadAppConfig() {
  const auto configPath = resolveConfigPath();

  try {
    drogon::app().loadConfigFile(configPath);
  } catch (const std::exception &e) {
    throw std::runtime_error("failed to load Drogon config file '" +
                             configPath + "': " + e.what());
  }

  auto config = loadAppConfigFromCurrentApp();
  config.filePath = configPath;
  return config;
}

AppConfig loadAppConfigFromCurrentApp() {
  auto config = loadAppConfigFromCustomConfig(drogon::app().getCustomConfig());
  config.filePath = resolveConfigPath();
  return config;
}

AppConfig loadAppConfigFromCustomConfig(const Json::Value &root) {
  AppConfig config;

  if (!root.isObject()) {
    throw std::runtime_error("custom_config must be a JSON object");
  }

  const auto &jwt = requireObject(root, "", "jwt");
  config.jwt.secret = requireString(jwt, "jwt", "secret");
  config.jwt.issuer = requireString(jwt, "jwt", "issuer");
  config.jwt.accessTtlSeconds =
      requirePositiveInt(jwt, "jwt", "access_ttl_seconds");
  config.jwt.refreshTtlSeconds =
      requirePositiveInt(jwt, "jwt", "refresh_ttl_seconds");

  const auto &redis = requireObject(root, "", "redis");
  config.redis.host = requireString(redis, "redis", "host");
  config.redis.port = requirePort(redis, "redis", "port");
  if (redis.isMember("password")) {
    if (!redis["password"].isString()) {
      throw std::runtime_error("custom_config.redis.password must be a string");
    }
    config.redis.password = redis["password"].asString();
  }

  const auto &minio = requireObject(root, "", "minio");
  config.minio.endpoint = requireString(minio, "minio", "endpoint");
  config.minio.accessKey = requireString(minio, "minio", "accessKey");
  config.minio.secretKey = requireString(minio, "minio", "secretKey");
  config.minio.bucket = requireString(minio, "minio", "bucket");
  config.minio.secure = requireBool(minio, "minio", "secure");

  applyEnvOverrides(config);
  validateConfig(config);
  return config;
}

} // namespace libsys
