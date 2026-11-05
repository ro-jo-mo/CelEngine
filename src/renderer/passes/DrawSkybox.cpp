#include "renderer/passes/DrawSkybox.h"

#include "renderer/Queues.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/asset/AssetServer.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/resource-management/PipelineBuilder.h"

void
Cel::Renderer::Passes::create_skybox_data(Resource<SkyboxData>& skybox,
                                          Resource<VulkanContext>& context,
                                          Resource<Asset::AssetServer>& server)
{
    skybox->cube = server->create_cube();
    skybox->texture = server->load_texture("../../assets/skybox.ktx2");
    skybox->pipeline = PipelineBuilder{ context->device }
                           .add_graphics_shader("skybox")
                           .set_cull_mode(VK_CULL_MODE_NONE)
                           .build();
}

void
Cel::Renderer::Passes::register_skybox_pass(
    Resource<SkyboxData>& skybox,
    Resource<Asset::AssetServer>& server,
    Resource<RenderGraph::Graph>& graph)
{
    auto pass = RenderGraph::PassBuilder{ Passes::skyboxPass }.set_queue(
        Queues::graphics);

    server->declare_scene_access(pass);

    pass.write_image(Passes::drawImage,
                     VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
                         VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .write_image(Passes::depthImage,
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                         VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    graph->add_pass(pass.build()).after(Passes::drawMeshPass);
}

void
Cel::Renderer::Passes::draw_skybox(
    Resource<SkyboxData>& skybox,
    Resource<Asset::AssetServer>& assetServer,
    ParallelResource<RenderGraph::PassServer>& passServer)
{
    VkCommandBuffer cmd;
    {
        cmd = passServer.write()->get_cmd_buffer(Passes::skyboxPass);
    }

    auto access = passServer.partial_read();

    auto guard = Utils::begin_rendering(
        cmd,
        access->get_resource(Passes::drawImage).imageView,
        access->get_resource(Passes::depthImage).imageView,
        access->get_extent(),
        VK_ATTACHMENT_LOAD_OP_LOAD);

    auto& cube = assetServer->get_primitive(skybox->cube);

    vkCmdBindPipeline(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox->pipeline.pipeline);

    const Asset::TextureAsset texture =
        assetServer->get_texture(skybox->texture);

    vkCmdPushConstants(cmd,
                       skybox->pipeline.pipelineLayout,
                       VK_SHADER_STAGE_ALL,
                       0,
                       sizeof(texture),
                       &texture);

    vkCmdDrawIndexed(
        cmd, cube.indexCount, 1, cube.firstIndex, cube.vertexOffset, 0);
}
