#include "plugins/InfrastructurePlugin.h"

#include "libsys/utils/AppConfig.h"
#include "libsys/utils/JwtUtils.h"
#include "libsys/utils/MinioClient.h"
#include "libsys/utils/PgClient.h"
#include "libsys/utils/RedisClient.h"

#include <drogon/drogon.h>

namespace libsys {

void InfrastructurePlugin::initAndStart(const Json::Value &config) {
  (void)config;

  const auto appConfig = loadAppConfigFromCurrentApp();

  JwtUtils::init(appConfig.jwt.secret, appConfig.jwt.issuer,
                 appConfig.jwt.accessTtlSeconds,
                 appConfig.jwt.refreshTtlSeconds);
  RedisClient::init(appConfig.redis.host, appConfig.redis.port,
                    appConfig.redis.password);
  PgClient::init(appConfig.postgres.host, appConfig.postgres.port,
                 appConfig.postgres.dbname, appConfig.postgres.user,
                 appConfig.postgres.password, appConfig.postgres.connNumber);
  MinioClient::init(appConfig.minio.endpoint, appConfig.minio.accessKey,
                    appConfig.minio.secretKey, appConfig.minio.bucket,
                    appConfig.minio.region, appConfig.minio.secure);

  LOG_INFO << "InfrastructurePlugin initialized";
}

void InfrastructurePlugin::shutdown() {
  MinioClient::destroy();
}

} // namespace libsys
