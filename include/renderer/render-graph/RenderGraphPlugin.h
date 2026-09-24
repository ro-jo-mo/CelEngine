#pragma once
#include "core/Plugin.h"

namespace Cel::Renderer {
struct RenderExtent;
class VulkanResourceManager;
}
namespace Cel::Renderer::RenderGraph {
class PassServer;
class Graph;

class RenderGraphPlugin final : public Plugin
{
  public:
    void build(SystemScheduler scheduler,
               ResourceManager& resourceManager) override;

    static void compile_graph(Resource<Graph>& graph,
                              Resource<VulkanResourceManager>& manager,
                              Resource<RenderExtent>& extent,
                              ParallelResource<PassServer>& server);
};

}