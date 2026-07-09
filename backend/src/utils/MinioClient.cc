#include "libsys/utils/MinioClient.h"

#include <miniocpp/args.h>
#include <miniocpp/client.h>
#include <miniocpp/providers.h>
#include <miniocpp/request.h>
#include <miniocpp/response.h>

#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>

namespace libsys {

namespace {

constexpr int kPresignExpireSeconds = 3600;

struct MinioConfig {
  std::string endpoint;
  std::string accessKey;
  std::string secretKey;
  std::string bucket;
  std::string region;
  bool secure{false};
};

std::mutex g_mutex;
MinioConfig g_config;
bool g_clientInitialized = false;

std::unique_ptr<minio::s3::BaseUrl> g_baseUrl;
std::unique_ptr<minio::creds::StaticProvider> g_provider;
std::unique_ptr<minio::s3::Client> g_client;

std::string trimSlashes(std::string value) {
  while (!value.empty() && value.front() == '/') {
    value.erase(value.begin());
  }
  while (!value.empty() && value.back() == '/') {
    value.pop_back();
  }
  return value;
}

MinioConfig normalizeConfig(const std::string &endpoint,
                            const std::string &accessKey,
                            const std::string &secretKey,
                            const std::string &bucket,
                            const std::string &region, bool secure) {
  if (endpoint.empty() || accessKey.empty() || secretKey.empty() ||
      bucket.empty()) {
    throw std::runtime_error(
        "MinIO config requires endpoint, accessKey, secretKey and bucket");
  }

  MinioConfig config;
  config.accessKey = accessKey;
  config.secretKey = secretKey;
  config.bucket = bucket;
  config.region = region.empty() ? "us-east-1" : region;
  config.secure = secure;

  std::string normalizedEndpoint = trimSlashes(endpoint);
  if (normalizedEndpoint.rfind("http://", 0) == 0) {
    normalizedEndpoint = normalizedEndpoint.substr(7);
    config.secure = false;
  } else if (normalizedEndpoint.rfind("https://", 0) == 0) {
    normalizedEndpoint = normalizedEndpoint.substr(8);
    config.secure = true;
  }

  if (normalizedEndpoint.find('/') != std::string::npos) {
    throw std::runtime_error("MinIO endpoint must not contain a path");
  }

  config.endpoint = normalizedEndpoint;
  return config;
}

void buildClient(const MinioConfig &config) {
  g_baseUrl = std::make_unique<minio::s3::BaseUrl>(
      config.endpoint, config.secure, config.region);
  g_provider = std::make_unique<minio::creds::StaticProvider>(config.accessKey,
                                                              config.secretKey);
  g_client = std::make_unique<minio::s3::Client>(*g_baseUrl, g_provider.get());
}

} // namespace

void MinioClient::init(const std::string &endpoint,
                       const std::string &accessKey,
                       const std::string &secretKey, const std::string &bucket,
                       const std::string &region, bool secure) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_config =
      normalizeConfig(endpoint, accessKey, secretKey, bucket, region, secure);
  buildClient(g_config);
  g_clientInitialized = true;
}

void MinioClient::shutdown() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_client.reset();
  g_provider.reset();
  g_baseUrl.reset();
  g_clientInitialized = false;
}

void MinioClient::destroy() { shutdown(); }

std::string MinioClient::putCover(const std::string &objectName,
                                  const std::string &contentType,
                                  const std::string &data) {
  if (objectName.empty()) {
    throw std::runtime_error("MinIO object name must not be empty");
  }

  std::lock_guard<std::mutex> lock(g_mutex);
  if (!g_clientInitialized || !g_client) {
    throw std::runtime_error("MinioClient not initialized");
  }

  const auto mimeType =
      contentType.empty() ? "application/octet-stream" : contentType;

  std::istringstream stream(data);
  minio::s3::PutObjectArgs args(stream, static_cast<long>(data.size()), 0);
  args.bucket = g_config.bucket;
  args.object = objectName;
  args.content_type = mimeType;

  auto resp = g_client->PutObject(args);
  if (!resp) {
    throw std::runtime_error("MinIO PUT failed: " + resp.Error().String());
  }
  return objectName;
}

std::string MinioClient::getUrl(const std::string &objectKey) {
  if (objectKey.empty()) {
    throw std::runtime_error("MinIO object key must not be empty");
  }

  std::lock_guard<std::mutex> lock(g_mutex);
  if (!g_clientInitialized || !g_client) {
    throw std::runtime_error("MinioClient not initialized");
  }

  minio::s3::GetPresignedObjectUrlArgs args;
  args.bucket = g_config.bucket;
  args.object = objectKey;
  args.method = minio::http::Method::kGet;
  args.expiry_seconds = kPresignExpireSeconds;

  auto resp = g_client->GetPresignedObjectUrl(args);
  if (!resp) {
    throw std::runtime_error("MinIO GET URL generation failed: " +
                             resp.Error().String());
  }
  return resp.url;
}

bool MinioClient::remove(const std::string &objectKey) {
  if (objectKey.empty()) {
    return false;
  }

  std::lock_guard<std::mutex> lock(g_mutex);
  if (!g_clientInitialized || !g_client) {
    throw std::runtime_error("MinioClient not initialized");
  }

  minio::s3::RemoveObjectArgs args;
  args.bucket = g_config.bucket;
  args.object = objectKey;

  auto resp = g_client->RemoveObject(args);
  if (!resp) {
    throw std::runtime_error("MinIO DELETE failed: " + resp.Error().String());
  }
  return true;
}

} // namespace libsys
