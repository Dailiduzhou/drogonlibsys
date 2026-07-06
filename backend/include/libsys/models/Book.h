#pragma once

#include <optional>
#include <string>

namespace libsys {

// 统一业务实体: 图书
struct Book {
  int64_t id{0};
  std::string title;
  std::string author;
  std::string description;
  std::string coverKey; // MinIO Object Key
  int stock{0};
  std::string createdAt;
  std::string updatedAt;
};

} // namespace libsys
