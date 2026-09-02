#pragma once

#include "ecs/Resource.h"

namespace Cel::Renderer {
class VulkanResourceManager;
}
namespace Cel::Renderer::RenderGraph {
class PassServer;
}
namespace Cel::Renderer::Assets {
class AssetServer;
}
namespace Cel::Renderer::RenderGraph {
class Graph;
}
namespace Cel::Renderer::Passes {

void
register_asset_upload_pass(Resource<Assets::AssetServer>& server,
                      Resource<VulkanResourceManager>& manager,
                      Resource<RenderGraph::Graph>& graph);

void
upload_assets(Resource<Assets::AssetServer>& assetServer,
              ParallelResource<RenderGraph::PassServer> passServer);

}