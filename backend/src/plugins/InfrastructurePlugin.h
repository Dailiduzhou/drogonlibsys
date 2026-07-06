#pragma once

#include <drogon/plugins/Plugin.h>

namespace libsys {

class InfrastructurePlugin : public drogon::Plugin<InfrastructurePlugin> {
public:
  void initAndStart(const Json::Value &config) override;
  void shutdown() override;
};

} // namespace libsys
