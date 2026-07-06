#pragma once

#include "libsys/models/Book.h"
#include <drogon/plugins/Plugin.h>
#include <string>
#include <vector>

namespace libsys {

// 全文检索服务: PostgreSQL tsvector + GIN 倒排索引, 结果可缓存
// 参见 AGENTS.md 2.3 全文搜索模块
class SearchService : public drogon::Plugin<SearchService> {
public:
  void initAndStart(const Json::Value &config) override;
  void shutdown() override;

  std::vector<Book> search(const std::string &query, int offset, int limit);
};

} // namespace libsys
