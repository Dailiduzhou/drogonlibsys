#include "services/OssService.h"
#include "libsys/utils/MinioClient.h"

#include <cctype>
#include <filesystem>
#include <string>

namespace libsys {

namespace {

std::string sanitizeFileName(const std::string &fileName) {
  auto sanitized = std::filesystem::path(fileName).filename().string();
  if (sanitized.empty() || sanitized == "." || sanitized == "..") {
    sanitized = "cover";
  }

  for (auto &ch : sanitized) {
    const auto uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch) || ch == '.' || ch == '-' || ch == '_') {
      continue;
    }
    ch = '-';
  }

  if (sanitized.empty()) {
    return "cover";
  }
  return sanitized;
}

} // namespace

void OssService::initAndStart(const Json::Value &config) { (void)config; }

void OssService::shutdown() {}

std::string OssService::uploadCover(int64_t bookId, const std::string &fileName,
                                    const std::string &contentType,
                                    const std::string &fileData) {
  // Object Key 规范: covers/<bookId>-<fileName>
  std::string objectName =
      "covers/" + std::to_string(bookId) + "-" + sanitizeFileName(fileName);
  return MinioClient::putCover(objectName, contentType, fileData);
}

std::string OssService::coverUrl(const std::string &objectKey) {
  return MinioClient::getUrl(objectKey);
}

bool OssService::deleteCover(const std::string &objectKey) {
  return MinioClient::remove(objectKey);
}

} // namespace libsys
