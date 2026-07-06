#include <drogon/drogon.h>

#include "libsys/utils/JwtUtils.h"
#include "libsys/utils/RedisClient.h"
#include "libsys/utils/MinioClient.h"
#include "libsys/utils/PgClient.h"
#include "filters/JwtAuthFilter.h"

#include <json/json.h>
#include <cstdlib>
#include <iostream>

int main() {
    auto &app = drogon::app();
    const char *configPath = std::getenv("LIBSYS_CONFIG_PATH");
    app.loadConfigFile(configPath != nullptr ? configPath : "config/config.json");

    auto &cfg = app.getCustomConfig();

    // JWT 配置
    const auto &jwt = cfg["jwt"];
    libsys::JwtUtils::init(
        jwt["secret"].asString(),
        jwt["issuer"].asString(),
        jwt["access_ttl_seconds"].asInt(),
        jwt["refresh_ttl_seconds"].asInt());

    // Redis 配置 (JWT 黑名单 + 业务缓存, hiredis 直连)
    const auto &rd = cfg["redis"];
    libsys::RedisClient::init(
        rd["host"].asString(),
        rd["port"].asInt(),
        rd["password"].asString());

    // PostgreSQL 数据访问层
    libsys::PgClient::init();

    // MinIO 对象存储
    const auto &mc = cfg["minio"];
    libsys::MinioClient::init(
        mc["endpoint"].asString(),
        mc["accessKey"].asString(),
        mc["secretKey"].asString(),
        mc["bucket"].asString(),
        mc["secure"].asBool());

    LOG_INFO << "drogonlibsys starting on port 8080 ...";

    app.run();
    return 0;
}
