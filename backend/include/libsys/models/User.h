#pragma once

#include <cstdint>
#include <string>

namespace libsys {

// 用户实体 (鉴权用)
struct User {
  int64_t id{0};
  std::string username;
  std::string password; // bcrypt 哈希
  std::string role;     // user / admin
  std::string createdAt;
  std::string updatedAt;
};

} // namespace libsys
