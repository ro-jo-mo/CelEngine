#pragma once

#include "HandleAllocator.h"
#include "ecs/Resource.h"

#include <vulkan/vulkan_core.h>

namespace Cel::Ui {
struct DebugUi;
}
namespace Cel::Renderer {
struct RenderExtent;
}
namespace Cel::Renderer::RenderGraph {
class Graph;
class PassServer;
}
namespace Cel::Renderer::Passes {

void
register_debug_ui_pass(Resource<Ui::DebugUi>& ui,
                       Resource<RenderExtent>& extent,
                       Resource<RenderGraph::Graph>& graph);

void
record_debug_ui_pass(ParallelResource<RenderGraph::PassServer>& passServer);

}
