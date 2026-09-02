#pragma once

#include "ecs/Resource.h"

namespace Cel::Renderer::RenderGraph {
class Graph;
class PassServer;
}
namespace Cel::Renderer::Assets {
class AssetServer;
}
namespace Cel::Renderer::Passes {

// Descriptors are set after we upload the asset data, so we can bind all images

void
register_bind_descriptors_pass(Resource<Assets::AssetServer>& server,
                               Resource<RenderGraph::Graph>& graph);

void
bind_descriptors(Resource<Assets::AssetServer>& assetServer,
                 ParallelResource<RenderGraph::PassServer> passServer);

}