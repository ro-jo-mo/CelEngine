#pragma once

#include "ecs/Resource.h"

namespace Cel::Renderer {
struct Swapchain;
}
namespace Cel::Renderer::RenderGraph {
class PassServer;
class Graph;
}
namespace Cel::Renderer::Passes {

// Swapchain is managed outside the render graph
void
register_present_pass(Resource<RenderGraph::Graph>& graph);

void
record_present_pass(Resource<Swapchain>& swapchain,
                    ParallelResource<RenderGraph::PassServer>& passServer);

}
