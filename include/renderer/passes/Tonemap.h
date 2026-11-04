#pragma once
#include "ecs/Resource.h"

namespace Cel::Renderer {
struct RenderExtent;
}
namespace Cel::Renderer::RenderGraph {
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
register_tonemap(Resource<RenderExtent>& extent);

void
tonemap_pass(Resource<Asset::SharedAssetData>& shared,
             ParallelResource<RenderGraph::PassServer>& passServer);

}