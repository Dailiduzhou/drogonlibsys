#pragma once

#include "libsys/models/Book.h"
#include "services/OssService.h"

#include <cstdlib>
#include <drogon/HttpRequest.h>
#include <drogon/drogon.h>
#include <json/json.h>

namespace libsys {

inline int64_t parseInt64Param(const drogon::HttpRequestPtr &req,
                               const std::string &name) {
  const auto value = req->getParameter(name);
  if (value.empty()) {
    return 0;
  }
  return std::strtoll(value.c_str(), nullptr, 10);
}

inline int64_t currentUserId(const drogon::HttpRequestPtr &req) {
  return req->getAttributes()->get<int64_t>("userId");
}

inline std::string currentRole(const drogon::HttpRequestPtr &req) {
  return req->getAttributes()->get<std::string>("role");
}

inline bool isAdmin(const drogon::HttpRequestPtr &req) {
  return currentRole(req) == "admin";
}

inline Json::Value bookJson(const Book &b) {
  Json::Value v;
  v["id"] = (Json::Int64)b.id;
  v["title"] = b.title;
  v["author"] = b.author;
  v["description"] = b.description;
  v["coverKey"] = b.coverKey;
  if (b.coverKey.empty()) {
    v["coverUrl"] = "";
  } else {
    try {
      auto *oss = drogon::app().getPlugin<OssService>();
      v["coverUrl"] = oss->coverUrl(b.coverKey);
    } catch (const std::exception &e) {
      LOG_WARN << "failed to resolve cover URL for key '" << b.coverKey
               << "': " << e.what();
      v["coverUrl"] = "";
    }
  }
  v["stock"] = b.stock;
  v["createdAt"] = b.createdAt;
  v["updatedAt"] = b.updatedAt;
  return v;
}

} // namespace libsys
