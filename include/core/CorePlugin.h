#pragma once

#include "Plugin.h"

namespace Cel {
class CorePlugin final : public Plugin
{
  public:
    void build(Scheduler scheduler, ResourceManager& resourceManager) override;
};
}
