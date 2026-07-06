#pragma once

#include <string>

namespace libsys {

// MinIO (S3 兼容) 对象存储封装
// 职责: 图书封面上传 / 下载 / 删除, 数据库仅保存 Object Key
class MinioClient {
public:
    static void init(const std::string &endpoint, const std::string &accessKey,
                     const std::string &secretKey, const std::string &bucket, bool secure);

    // 上传封面图片, 返回 Object Key
    // objectName 形如: covers/<bookId>-<uuid>.jpg
    static std::string putCover(const std::string &objectName,
                                const std::string &contentType,
                                const std::string &data);

    // 获取对象的访问 URL
    static std::string getUrl(const std::string &objectKey);

    static bool remove(const std::string &objectKey);
};

}  // namespace libsys
