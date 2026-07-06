#include "services/OssService.h"
#include "libsys/utils/MinioClient.h"

#include <string>

namespace libsys {

std::string OssService::uploadCover(int64_t bookId,
                                    const std::string &fileName,
                                    const std::string &contentType,
                                    const std::string &fileData) {
    // Object Key 规范: covers/<bookId>-<fileName>
    std::string objectName = "covers/" + std::to_string(bookId) + "-" + fileName;
    return MinioClient::putCover(objectName, contentType, fileData);
}

std::string OssService::coverUrl(const std::string &objectKey) {
    return MinioClient::getUrl(objectKey);
}

}  // namespace libsys
