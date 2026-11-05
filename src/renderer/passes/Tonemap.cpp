#include "renderer/passes/Tonemap.h"

#include "ecs/Resource.h"
#include "renderer/Queues.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/render-graph/RenderGraph.h"
#include "renderer/resource-management/PipelineBuilder.h"

void
Cel::Renderer::Passes::register_tonemap(Resource<RenderExtent>& extent,
                                        Resource<RenderGraph::Graph>& graph)
{
    // Read from the drawn image and output to the final image
    auto pass =
        RenderGraph::PassBuilder(Passes::tonemapPass)
            .set_queue(Queues::graphics)
            .read_image(Passes::drawImage,
                        VK_ACCESS_2_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            .create_image(Passes::finalImage,
                          false,
                          VK_FORMAT_R8G8B8A8_UNORM,
                          extent->extent3d(),
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT)
            .write_image(Passes::finalImage,
                         VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .build();

    graph->add_pass(pass).after(Passes::drawMeshPass);
}

void
Cel::Renderer::Passes::tonemap_pass(
    ParallelResource<RenderGraph::PassServer>& passServer)
{
    VkCommandBuffer cmd;

    {
        cmd = passServer.write()->get_cmd_buffer(Passes::tonemapPass);
    }

    auto access = passServer.partial_read();

    const static auto pipeline =
        PipelineBuilder(access->get_device())
            .add_graphics_shader("tonemap")
            .disable_depth_test()
            .set_color_attachment(VK_FORMAT_R8G8B8A8_UNORM)
            .set_cull_mode(VK_CULL_MODE_NONE)
            .build();

    auto guard = Utils::begin_rendering_no_depth(
        cmd,
        access->get_resource(Passes::finalImage).imageView,
        access->get_extent());

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    auto imageIndice = access->get_texture_index(Passes::drawImage);

    vkCmdPushConstants(cmd,
                       pipeline.pipelineLayout,
                       VK_SHADER_STAGE_ALL,
                       0,
                       sizeof(uint32_t),
                       &imageIndice);

    vkCmdDraw(cmd, 3, 1, 0, 0);
}
