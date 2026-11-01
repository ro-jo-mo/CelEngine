#pragma once
#include "ecs/Resource.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/render-graph/RenderGraph.h"

namespace Cel::Renderer::Passes {

struct SkyboxData
{
    Pipeline pipeline;
    Handle<Asset::Primitive> cube;
    Handle<Asset::TextureAsset> texture;
};

// Needs.
// A cube mesh.
// Pipeline

void
create_skybox_data(Resource<SkyboxData>& skybox,
                   Resource<VulkanContext>& context,
                   Resource<Asset::AssetServer>& server);

void
register_skybox_pass(Resource<SkyboxData>& skybox,
                     Resource<Asset::AssetServer>& server,
                     Resource<RenderGraph::Graph>& graph);

void
draw_skybox(Resource<SkyboxData>& skybox,
            Resource<Asset::AssetServer>& assetServer,
            ParallelResource<RenderGraph::PassServer>& passServer);

}
