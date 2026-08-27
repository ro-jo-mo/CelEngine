#pragma once
#include "ecs/Resource.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/render-graph/RenderGraph.h"

namespace Cel::Renderer::Passes {

struct SkyboxData
{
    Pipeline pipeline;
};

void
create_skybox_data();

void
add_skybox_pass(Resource<RenderGraph::Graph>& graph);

void
draw_skybox(ParallelResource<RenderGraph::PassServer>& server);

}