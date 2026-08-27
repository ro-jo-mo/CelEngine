#include "renderer/passes/DrawSkybox.h"

#include "renderer/resource-management/PipelineBuilder.h"

void
Cel::Renderer::Passes::create_skybox_data()
{
}

void
Cel::Renderer::Passes::draw_skybox(
    ParallelResource<RenderGraph::PassServer>& server,
    Resource<SkyboxData>& data)
{

    static auto pipeline =
        PipelineBuilder(context->device)
            .add_shader_module("../../shaders/skybox.vert.spv",
                               VK_SHADER_STAGE_VERTEX_BIT)
            .add_shader_module("../../shaders/skybox.frag.spv",
                               VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();
}
