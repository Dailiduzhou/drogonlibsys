#include "libsys/utils/MinioClient.h"

#include <curl/curl.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace libsys {

namespace {

constexpr const char *kAwsAlgorithm = "AWS4-HMAC-SHA256";
constexpr const char *kAwsService = "s3";
constexpr const char *kAwsRegion = "us-east-1";
constexpr int kPresignExpireSeconds = 3600;

struct MinioConfig {
  std::string endpoint;
  std::string hostHeader;
  std::string baseUrl;
  std::string accessKey;
  std::string secretKey;
  std::string bucket;
  bool secure{false};
};

struct TimestampParts {
  std::string amzDate;
  std::string shortDate;
};

struct SignedRequest {
  std::string canonicalUri;
  std::vector<std::pair<std::string, std::string>> headers;
};

std::mutex g_mutex;
MinioConfig g_config;
bool g_initialized = false;
std::once_flag g_curlInitOnce;

size_t appendResponse(char *ptr, size_t size, size_t nmemb, void *userdata) {
  auto *buffer = static_cast<std::string *>(userdata);
  buffer->append(ptr, size * nmemb);
  return size * nmemb;
}

void ensureCurlInitialized() {
  std::call_once(g_curlInitOnce, []() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
      throw std::runtime_error("curl_global_init failed");
    }
    std::atexit([]() { curl_global_cleanup(); });
  });
}

MinioConfig currentConfig() {
  std::lock_guard<std::mutex> lock(g_mutex);
  if (!g_initialized) {
    throw std::runtime_error("MinioClient not initialized");
  }
  return g_config;
}

std::string trimSlashes(std::string value) {
  while (!value.empty() && value.front() == '/') {
    value.erase(value.begin());
  }
  while (!value.empty() && value.back() == '/') {
    value.pop_back();
  }
  return value;
}

std::string hexEncode(const unsigned char *data, size_t len) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out;
  out.resize(len * 2);
  for (size_t i = 0; i < len; ++i) {
    out[i * 2] = kHex[(data[i] >> 4) & 0x0F];
    out[i * 2 + 1] = kHex[data[i] & 0x0F];
  }
  return out;
}

std::string sha256Hex(const std::string &data) {
  unsigned char digest[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char *>(data.data()), data.size(),
         digest);
  return hexEncode(digest, sizeof(digest));
}

std::string hmacSha256(std::string_view key, std::string_view data) {
  unsigned int len = 0;
  std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
  HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
       reinterpret_cast<const unsigned char *>(data.data()), data.size(),
       digest.data(), &len);
  return std::string(reinterpret_cast<const char *>(digest.data()), len);
}

std::string percentEncode(std::string_view text, bool encodeSlash) {
  std::ostringstream out;
  out.fill('0');
  out << std::uppercase;
  for (unsigned char ch : text) {
    if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~' ||
        (!encodeSlash && ch == '/')) {
      out << static_cast<char>(ch);
    } else {
      out << '%' << std::setw(2) << std::hex << static_cast<int>(ch)
          << std::dec;
    }
  }
  return out.str();
}

TimestampParts makeTimestamp() {
  std::time_t now = std::time(nullptr);
  std::tm utc{};
#if defined(_WIN32)
  gmtime_s(&utc, &now);
#else
  gmtime_r(&now, &utc);
#endif

  char shortDate[9];
  char amzDate[17];
  std::strftime(shortDate, sizeof(shortDate), "%Y%m%d", &utc);
  std::strftime(amzDate, sizeof(amzDate), "%Y%m%dT%H%M%SZ", &utc);
  return {amzDate, shortDate};
}

std::string credentialScope(const TimestampParts &ts) {
  return ts.shortDate + "/" + kAwsRegion + "/" + kAwsService + "/aws4_request";
}

std::string canonicalUri(const std::string &bucket,
                         const std::string &objectKey) {
  return "/" + percentEncode(bucket, true) + "/" +
         percentEncode(trimSlashes(objectKey), false);
}

std::string signingKey(const MinioConfig &config, const TimestampParts &ts) {
  const auto dateKey = hmacSha256("AWS4" + config.secretKey, ts.shortDate);
  const auto regionKey = hmacSha256(dateKey, kAwsRegion);
  const auto serviceKey = hmacSha256(regionKey, kAwsService);
  return hmacSha256(serviceKey, "aws4_request");
}

std::string buildCanonicalHeaders(
    const std::vector<std::pair<std::string, std::string>> &headers) {
  std::string out;
  for (const auto &[name, value] : headers) {
    out += name;
    out += ':';
    out += value;
    out += '\n';
  }
  return out;
}

std::string joinSignedHeaders(
    const std::vector<std::pair<std::string, std::string>> &headers) {
  std::string out;
  for (size_t i = 0; i < headers.size(); ++i) {
    if (i > 0) {
      out += ';';
    }
    out += headers[i].first;
  }
  return out;
}

std::string buildAuthorization(const MinioConfig &config,
                               const TimestampParts &ts,
                               const std::string &canonicalRequestHash,
                               const std::string &signedHeaders) {
  const auto scope = credentialScope(ts);
  const std::string stringToSign = std::string(kAwsAlgorithm) + "\n" +
                                   ts.amzDate + "\n" + scope + "\n" +
                                   canonicalRequestHash;
  const auto signatureBytes = hmacSha256(signingKey(config, ts), stringToSign);
  const auto signature =
      hexEncode(reinterpret_cast<const unsigned char *>(signatureBytes.data()),
                signatureBytes.size());
  return std::string(kAwsAlgorithm) + " Credential=" + config.accessKey + "/" +
         scope + ", SignedHeaders=" + signedHeaders +
         ", Signature=" + signature;
}

SignedRequest signRequest(const MinioConfig &config, std::string_view method,
                          const std::string &objectKey,
                          const std::string &payloadHash,
                          std::string_view contentType) {
  const auto ts = makeTimestamp();
  SignedRequest req;
  req.canonicalUri = canonicalUri(config.bucket, objectKey);
  req.headers = {
      {"host", config.hostHeader},
      {"x-amz-content-sha256", payloadHash},
      {"x-amz-date", ts.amzDate},
  };
  if (!contentType.empty()) {
    req.headers.emplace_back("content-type", std::string(contentType));
  }
  std::sort(
      req.headers.begin(), req.headers.end(),
      [](const auto &lhs, const auto &rhs) { return lhs.first < rhs.first; });

  const auto canonicalHeaders = buildCanonicalHeaders(req.headers);
  const auto signedHeaders = joinSignedHeaders(req.headers);
  const std::string canonicalRequest =
      std::string(method) + "\n" + req.canonicalUri + "\n\n" +
      canonicalHeaders + "\n" + signedHeaders + "\n" + payloadHash;
  const auto canonicalRequestHash = sha256Hex(canonicalRequest);

  req.headers.emplace_back(
      "authorization",
      buildAuthorization(config, ts, canonicalRequestHash, signedHeaders));
  return req;
}

std::string buildPresignedUrl(const MinioConfig &config,
                              const std::string &objectKey) {
  const auto ts = makeTimestamp();
  const auto uri = canonicalUri(config.bucket, objectKey);

  std::vector<std::pair<std::string, std::string>> query = {
      {"X-Amz-Algorithm", kAwsAlgorithm},
      {"X-Amz-Credential", config.accessKey + "/" + credentialScope(ts)},
      {"X-Amz-Date", ts.amzDate},
      {"X-Amz-Expires", std::to_string(kPresignExpireSeconds)},
      {"X-Amz-SignedHeaders", "host"},
  };
  std::sort(query.begin(), query.end(), [](const auto &lhs, const auto &rhs) {
    if (lhs.first != rhs.first) {
      return lhs.first < rhs.first;
    }
    return lhs.second < rhs.second;
  });

  std::string canonicalQuery;
  for (size_t i = 0; i < query.size(); ++i) {
    if (i > 0) {
      canonicalQuery += '&';
    }
    canonicalQuery += percentEncode(query[i].first, true) + "=" +
                      percentEncode(query[i].second, true);
  }

  const std::string canonicalRequest = "GET\n" + uri + "\n" + canonicalQuery +
                                       "\n" + "host:" + config.hostHeader +
                                       "\n\nhost\nUNSIGNED-PAYLOAD";
  const std::string stringToSign = std::string(kAwsAlgorithm) + "\n" +
                                   ts.amzDate + "\n" + credentialScope(ts) +
                                   "\n" + sha256Hex(canonicalRequest);
  const auto signatureBytes = hmacSha256(signingKey(config, ts), stringToSign);
  const auto signature =
      hexEncode(reinterpret_cast<const unsigned char *>(signatureBytes.data()),
                signatureBytes.size());
  return config.baseUrl + uri + "?" + canonicalQuery +
         "&X-Amz-Signature=" + signature;
}

void performRequest(const MinioConfig &config, std::string_view method,
                    const std::string &objectKey, std::string_view contentType,
                    const std::string &body) {
  const auto payloadHash = sha256Hex(body);
  const auto signedRequest =
      signRequest(config, method, objectKey, payloadHash, contentType);

  CURL *curl = curl_easy_init();
  if (curl == nullptr) {
    throw std::runtime_error("failed to create CURL handle");
  }

  struct curl_slist *headers = nullptr;
  std::string responseBody;
  const std::string url = config.baseUrl + signedRequest.canonicalUri;
  const std::string methodString(method);

  for (const auto &[name, value] : signedRequest.headers) {
    headers = curl_slist_append(headers, (name + ": " + value).c_str());
  }
  headers = curl_slist_append(headers, "Expect:");

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, methodString.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, appendResponse);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  if (method == "PUT") {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,
                     static_cast<curl_off_t>(body.size()));
  }

  const CURLcode code = curl_easy_perform(curl);
  long status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (code != CURLE_OK) {
    throw std::runtime_error("MinIO " + methodString +
                             " failed: " + curl_easy_strerror(code));
  }
  if (status < 200 || status >= 300) {
    std::ostringstream out;
    out << "MinIO " << methodString << " failed with HTTP " << status;
    if (!responseBody.empty()) {
      out << ": " << responseBody;
    }
    throw std::runtime_error(out.str());
  }
}

MinioConfig normalizeConfig(const std::string &endpoint,
                            const std::string &accessKey,
                            const std::string &secretKey,
                            const std::string &bucket, bool secure) {
  if (endpoint.empty() || accessKey.empty() || secretKey.empty() ||
      bucket.empty()) {
    throw std::runtime_error(
        "MinIO config requires endpoint, accessKey, secretKey and bucket");
  }

  MinioConfig config;
  config.accessKey = accessKey;
  config.secretKey = secretKey;
  config.bucket = bucket;
  config.secure = secure;

  std::string normalizedEndpoint = trimSlashes(endpoint);
  if (normalizedEndpoint.rfind("http://", 0) == 0) {
    normalizedEndpoint = normalizedEndpoint.substr(std::strlen("http://"));
    config.secure = false;
  } else if (normalizedEndpoint.rfind("https://", 0) == 0) {
    normalizedEndpoint = normalizedEndpoint.substr(std::strlen("https://"));
    config.secure = true;
  }
  if (normalizedEndpoint.find('/') != std::string::npos) {
    throw std::runtime_error("MinIO endpoint must not contain a path");
  }

  config.endpoint = normalizedEndpoint;
  config.hostHeader = normalizedEndpoint;
  config.baseUrl =
      std::string(config.secure ? "https://" : "http://") + normalizedEndpoint;
  return config;
}

} // namespace

void MinioClient::init(const std::string &endpoint,
                       const std::string &accessKey,
                       const std::string &secretKey, const std::string &bucket,
                       bool secure) {
  ensureCurlInitialized();
  std::lock_guard<std::mutex> lock(g_mutex);
  g_config = normalizeConfig(endpoint, accessKey, secretKey, bucket, secure);
  g_initialized = true;
}

std::string MinioClient::putCover(const std::string &objectName,
                                  const std::string &contentType,
                                  const std::string &data) {
  if (objectName.empty()) {
    throw std::runtime_error("MinIO object name must not be empty");
  }
  const auto mimeType =
      contentType.empty() ? "application/octet-stream" : contentType;
  performRequest(currentConfig(), "PUT", objectName, mimeType, data);
  return objectName;
}

std::string MinioClient::getUrl(const std::string &objectKey) {
  if (objectKey.empty()) {
    throw std::runtime_error("MinIO object key must not be empty");
  }
  return buildPresignedUrl(currentConfig(), objectKey);
}

bool MinioClient::remove(const std::string &objectKey) {
  if (objectKey.empty()) {
    return false;
  }
  performRequest(currentConfig(), "DELETE", objectKey, "", "");
  return true;
}

} // namespace libsys
