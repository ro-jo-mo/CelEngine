#pragma once
#include "core/Plugin.h"

namespace Cel::Renderer::RenderGraph {

class RenderGraphPlugin final : public Plugin
{
  public:
    void build(SystemScheduler scheduler,
               ResourceManager& resourceManager) override;
};

}