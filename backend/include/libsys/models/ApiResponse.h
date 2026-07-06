#pragma once

#include <drogon/HttpResponse.h>
#include <json/json.h>

namespace libsys {

// 统一 API 响应封装
// 成功: {"code":0,"msg":"ok","data":{...}}
// 失败: {"code":<非0>,"msg":"<错误描述>","data":null}
class ApiResponse {
public:
  static drogon::HttpResponsePtr ok(const Json::Value &data = Json::Value());
  static drogon::HttpResponsePtr fail(int code, const std::string &msg);
};

} // namespace libsys
