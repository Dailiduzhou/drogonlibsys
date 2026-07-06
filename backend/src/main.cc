#include <drogon/drogon.h>

#include "libsys/utils/JwtUtils.h"
#include "libsys/utils/AppConfig.h"
#include "libsys/utils/MinioClient.h"
#include "libsys/utils/PgClient.h"
#include "libsys/utils/RedisClient.h"
#include "filters/JwtAuthFilter.h"

#include <exception>
#include <iostream>

int main() {
    try {
        const auto config = libsys::loadAppConfig();

        // JWT 配置
        libsys::JwtUtils::init(config.jwt.secret,
                               config.jwt.issuer,
                               config.jwt.accessTtlSeconds,
                               config.jwt.refreshTtlSeconds);

        // Redis 配置 (JWT 黑名单 + 业务缓存, hiredis 直连)
        libsys::RedisClient::init(config.redis.host,
                                  config.redis.port,
                                  config.redis.password);

        // PostgreSQL 数据访问层
        libsys::PgClient::init();

        // MinIO 对象存储
        libsys::MinioClient::init(config.minio.endpoint,
                                  config.minio.accessKey,
                                  config.minio.secretKey,
                                  config.minio.bucket,
                                  config.minio.secure);

        LOG_INFO << "Loaded config from " << config.filePath;
        LOG_INFO << "drogonlibsys starting on port 8080 ...";

        drogon::app().run();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Startup failed: " << e.what() << '\n';
        return 1;
    }
}
