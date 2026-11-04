#pragma once
#include "core/App.h"
#include "core/Plugin.h"
#include "renderer/asset/AssetTypes.h"

namespace Cel::Renderer {
namespace Asset {
class AssetServer;
}
struct RenderExtent;
struct Swapchain;
class VulkanResourceManager;
}
namespace Cel::Renderer::RenderGraph {
class PassServer;
class Graph;

class RenderGraphPlugin
{
  public:
    void build(PluginManager& manager);

    static void compile_graph(Resource<Graph>& graph,
                              Resource<VulkanResourceManager>& manager,
                              Resource<RenderExtent>& extent,
                              Resource<Asset::SharedAssetData>& shared,
                              ParallelResource<PassServer>& passServer);

    static void acquire_swapchain_image(
        Resource<Swapchain>& swapchain,
        ParallelResource<PassServer>& passServer);
};

}