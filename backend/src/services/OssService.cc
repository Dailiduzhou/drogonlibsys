#include "services/OssService.h"
#include "libsys/utils/MinioClient.h"

#include <array>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

namespace libsys {

namespace {

std::string generateUuidV4() {
  static thread_local std::mt19937_64 rng{std::random_device{}()};
  std::uniform_int_distribution<int> dist(0, 255);

  std::array<unsigned char, 16> bytes{};
  for (auto &byte : bytes) {
    byte = static_cast<unsigned char>(dist(rng));
  }
  bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0f) | 0x40);
  bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3f) | 0x80);

  std::ostringstream ss;
  ss << std::hex << std::setfill('0');
  for (size_t i = 0; i < bytes.size(); ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      ss << '-';
    }
    ss << std::setw(2) << static_cast<int>(bytes[i]);
  }
  return ss.str();
}

int64_t currentTimestampMillis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::string sanitizeExtension(const std::string &fileName) {
  auto ext = std::filesystem::path(fileName).filename().extension().string();
  if (ext.size() <= 1 || ext.size() > 16 || ext[0] != '.') {
    return "";
  }

  for (size_t i = 1; i < ext.size(); ++i) {
    const auto uch = static_cast<unsigned char>(ext[i]);
    if (!std::isalnum(uch)) {
      return "";
    }
    ext[i] = static_cast<char>(std::tolower(uch));
  }
  return ext;
}

} // namespace

void OssService::initAndStart(const Json::Value &config) { (void)config; }

void OssService::shutdown() {}

std::string OssService::uploadCover(int64_t bookId, const std::string &fileName,
                                    const std::string &contentType,
                                    const std::string &fileData) {
  (void)bookId;
  // Object Key 规范: covers/<uuid>-<timestampMillis><ext>
  std::string objectName =
      "covers/" + generateUuidV4() + "-" +
      std::to_string(currentTimestampMillis()) + sanitizeExtension(fileName);
  return MinioClient::putCover(objectName, contentType, fileData);
}

std::string OssService::coverUrl(const std::string &objectKey) {
  return MinioClient::getUrl(objectKey);
}

bool OssService::deleteCover(const std::string &objectKey) {
  return MinioClient::remove(objectKey);
}

} // namespace libsys
