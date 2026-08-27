#pragma once
#include "core/Plugin.h"

namespace Cel::Renderer::Passes {

class PassPlugin final : public Plugin
{
  public:
    void build(SystemScheduler scheduler,
               ResourceManager& resourceManager) override;
};

}