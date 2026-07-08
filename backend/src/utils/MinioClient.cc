#include "libsys/utils/MinioClient.h"

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/signer/AWSAuthV4Signer.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpTypes.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <memory>
#include <mutex>
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
std::shared_ptr<Aws::S3::S3Client> g_client;
MinioConfig g_config;
bool g_clientInitialized = false;

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

std::shared_ptr<Aws::S3::S3Client> buildClient(const MinioConfig &config) {
  Aws::Client::ClientConfiguration clientConfig;
  clientConfig.region = config.region.c_str();
  clientConfig.scheme =
      config.secure ? Aws::Http::Scheme::HTTPS : Aws::Http::Scheme::HTTP;
  clientConfig.endpointOverride = config.endpoint.c_str();
  clientConfig.connectTimeoutMs = 10000;
  clientConfig.requestTimeoutMs = 30000;
  clientConfig.verifySSL = config.secure;

  Aws::Auth::AWSCredentials credentials(config.accessKey.c_str(),
                                        config.secretKey.c_str());

  return std::make_shared<Aws::S3::S3Client>(
      credentials, clientConfig,
      Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::RequestDependent,
      false);
}

struct ClientState {
  std::shared_ptr<Aws::S3::S3Client> client;
  MinioConfig config;
};

ClientState currentState() {
  std::lock_guard<std::mutex> lock(g_mutex);
  if (!g_clientInitialized || !g_client) {
    throw std::runtime_error("MinioClient not initialized");
  }
  return {g_client, g_config};
}

std::runtime_error
s3Error(const std::string &action,
        const Aws::Client::AWSError<Aws::S3::S3Errors> &error) {
  return std::runtime_error("MinIO " + action +
                            " failed: " + error.GetExceptionName() + ": " +
                            error.GetMessage());
}

} // namespace

void MinioClient::init(const std::string &endpoint,
                       const std::string &accessKey,
                       const std::string &secretKey, const std::string &bucket,
                       const std::string &region, bool secure) {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_config =
      normalizeConfig(endpoint, accessKey, secretKey, bucket, region, secure);
  g_client = buildClient(g_config);
  g_clientInitialized = true;
}

void MinioClient::shutdown() {
  std::lock_guard<std::mutex> lock(g_mutex);
  g_client.reset();
  g_clientInitialized = false;
}

std::string MinioClient::putCover(const std::string &objectName,
                                  const std::string &contentType,
                                  const std::string &data) {
  if (objectName.empty()) {
    throw std::runtime_error("MinIO object name must not be empty");
  }

  const auto state = currentState();
  const auto mimeType =
      contentType.empty() ? "application/octet-stream" : contentType;

  Aws::S3::Model::PutObjectRequest request;
  request.SetBucket(state.config.bucket.c_str());
  request.SetKey(objectName.c_str());
  request.SetContentType(mimeType.c_str());
  request.SetContentLength(static_cast<long long>(data.size()));

  auto body = Aws::MakeShared<Aws::StringStream>("MinioClient::putCoverBody");
  body->write(data.data(), static_cast<std::streamsize>(data.size()));
  body->seekg(0, std::ios_base::beg);
  request.SetBody(body);

  auto outcome = state.client->PutObject(request);
  if (!outcome.IsSuccess()) {
    throw s3Error("PUT", outcome.GetError());
  }
  return objectName;
}

std::string MinioClient::getUrl(const std::string &objectKey) {
  if (objectKey.empty()) {
    throw std::runtime_error("MinIO object key must not be empty");
  }

  const auto state = currentState();
  const auto url = state.client->GeneratePresignedUrl(
      state.config.bucket.c_str(), objectKey.c_str(),
      Aws::Http::HttpMethod::HTTP_GET, kPresignExpireSeconds);
  if (url.empty()) {
    throw std::runtime_error("MinIO GET URL generation failed");
  }
  return {url.c_str()};
}

bool MinioClient::remove(const std::string &objectKey) {
  if (objectKey.empty()) {
    return false;
  }

  const auto state = currentState();

  Aws::S3::Model::DeleteObjectRequest request;
  request.SetBucket(state.config.bucket.c_str());
  request.SetKey(objectKey.c_str());

  auto outcome = state.client->DeleteObject(request);
  if (!outcome.IsSuccess()) {
    throw s3Error("DELETE", outcome.GetError());
  }
  return true;
}

} // namespace libsys
