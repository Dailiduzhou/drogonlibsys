#pragma once

#include <string>

namespace libsys {

// OSS 存储服务: 图书封面上传至 MinIO
// 参见 AGENTS.md 2.2 封面文件上传
class OssService {
public:
  // 上传封面, 返回 Object Key (入库保存)
  std::string uploadCover(int64_t bookId, const std::string &fileName,
                          const std::string &contentType,
                          const std::string &fileData);

  // 获取封面访问 URL
  std::string coverUrl(const std::string &objectKey);

  // 删除封面对象
  bool deleteCover(const std::string &objectKey);
};

} // namespace libsys
