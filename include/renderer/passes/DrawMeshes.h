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
namespace Cel::Renderer::Asset {
struct Mesh;
struct Material;
class AssetServer;
}
namespace Cel::Renderer::Passes {
struct SceneData;

void
register_indirect_draw_data_pass(Resource<RenderGraph::Graph>& graph);

void
create_indirect_draw_data(
    Query<With<Entity, Handle<Asset::Mesh>, Handle<Asset::Material>>>&
        renderables,
    ParallelResource<RenderGraph::PassServer>& passServer,
    Resource<Asset::AssetServer>& assetServer,
    Resource<SceneData>& scene);

void
register_draw_mesh_pass(Resource<Asset::AssetServer>& server,
                        Resource<RenderGraph::Graph>& graph,
                        Resource<RenderExtent>& extent);

void
draw_mesh(Query<With<Entity, Handle<Asset::Mesh>, Handle<Asset::Material>>>&
              renderables,
          ParallelResource<RenderGraph::PassServer>& passServer);

}