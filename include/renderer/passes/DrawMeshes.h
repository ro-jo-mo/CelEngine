#pragma once

#include "common/Handle.h"
#include "ecs/Query.h"
#include "ecs/Resource.h"

namespace Cel::Renderer {
struct RenderExtent;
class VulkanResourceManager;
}
namespace Cel::Renderer::RenderGraph {
class PassServer;
class Graph;
}
namespace Cel::Renderer::Assets {
struct Mesh;
struct Material;
class AssetServer;
}
namespace Cel::Renderer::Passes {
struct SceneData;

// Passes will create their own indirect buffers?
// Passes will add to the indirect mega buffer during their registration phase
// Then the mega buffer is uploaded in a pass, and then indirect passes can come
// after This pass registration simpyl needs to occur prior to the indirect
// buffer upload registration
// However, ideally as it's per frame it would be handled by the render graph
// Perhaps a version of the mega buffer that doesn't own a buffer?
// Really it's just reset each frame
// In this case I think it's fine that we rely on traditional scheduling to
// ensure our passes upload their indirect commands to the mega buffer before
// its uploaded during a pass

void
register_indirect_draw_data_pass(Resource<RenderGraph::Graph>& graph);

void
create_indirect_draw_data(
    Query<With<Entity, Handle<Assets::Mesh>, Handle<Assets::Material>>>&
        renderables,
    ParallelResource<RenderGraph::PassServer>& passServer,
    Resource<Assets::AssetServer>& assetServer,
    Resource<SceneData>& scene);

void
register_draw_mesh_pass(Resource<Assets::AssetServer>& server,
                        Resource<RenderGraph::Graph>& graph,
                        Resource<RenderExtent>& extent);

void
draw_mesh(Query<With<Entity, Handle<Assets::Mesh>, Handle<Assets::Material>>>&
              renderables,
          ParallelResource<RenderGraph::PassServer>& passServer);

}