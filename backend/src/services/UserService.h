#pragma once

#include "libsys/models/User.h"
#include <cstdint>
#include <drogon/plugins/Plugin.h>
#include <optional>
#include <vector>

namespace libsys {

// 用户管理服务: 列表 / 详情 / 删除 (管理员维护)
// 删除前会校验该用户是否仍有未还书 (status='borrowed') 的借阅记录:
// 若有则阻止删除, 避免级联清理丢失审计/库存对账依据; 仅当全部归还后才允许
// 删除 (此时 ON DELETE CASCADE 仅清理历史已还记录).
class UserService : public drogon::Plugin<UserService> {
public:
  void initAndStart(const Json::Value &config) override;
  void shutdown() override;

  std::optional<User> getUser(int64_t id);
  std::vector<User> listUsers(int64_t offset, int64_t limit);
  bool deleteUser(int64_t id);

  // 全表 admin 数量, 用于防止删除最后一个管理员
  int64_t countAdmins();

  // 该用户未还书数量; DB 异常返回 nullopt (调用方应拒绝删除以 fail-closed).
  std::optional<int64_t> countActiveLoansByUser(int64_t userId);
};

} // namespace libsys
