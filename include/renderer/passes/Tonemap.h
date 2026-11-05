#pragma once
#include "ecs/Resource.h"

namespace Cel::Renderer {
struct RenderExtent;
}
namespace Cel::Renderer::RenderGraph {
class Graph;
class PassServer;
}
namespace Cel::Renderer::Asset {
struct SharedAssetData;
}
namespace Cel::Renderer::Passes {

// I need to add the draw image to the texture cache, ideally each frame (to
// accommodate swapchain resizing)
// vkCmdDraw(3,1,0)

void
register_tonemap(Resource<RenderExtent>& extent,
                 Resource<RenderGraph::Graph>& graph);

void
tonemap_pass(ParallelResource<RenderGraph::PassServer>& passServer);

}