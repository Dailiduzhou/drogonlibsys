#pragma once

#include "libsys/models/Book.h"
#include <drogon/plugins/Plugin.h>
#include <string>
#include <vector>

namespace libsys {

// 搜索服务: PostgreSQL pg_trgm 索引检索, 结果可缓存
// 参见 AGENTS.md 2.3 全文搜索模块
class SearchService : public drogon::Plugin<SearchService> {
public:
  void initAndStart(const Json::Value &config) override;
  void shutdown() override;

  std::vector<Book> search(const std::string &query, int64_t offset,
                           int64_t limit);
};

} // namespace libsys
