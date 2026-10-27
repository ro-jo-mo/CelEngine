#pragma once
#include "core/App.h"
#include "core/Plugin.h"
#include "renderer/asset/AssetTypes.h"

namespace Cel::Renderer {
namespace Asset {
class AssetServer;
}
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
                              Resource<Asset::SharedAssetData>& shared,
                              ParallelResource<PassServer>& passServer);
};

}