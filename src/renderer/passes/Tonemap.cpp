#include "renderer/passes/Tonemap.h"

#include "ecs/Resource.h"
#include "renderer/Queues.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/resource-management/PipelineBuilder.h"

void
Cel::Renderer::Passes::register_tonemap(Resource<RenderExtent>& extent)
{
    // Read from the drawn image and output to the final image
    auto pass = RenderGraph::PassBuilder(Passes::tonemapPass)
                    .set_queue(Queues::graphics)
                    .read_image(Passes::drawImage,
                                VK_ACCESS_2_SHADER_READ_BIT,
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                    .create_image(Passes::finalImage,
                                  false,
                                  VK_FORMAT_R8G8B8A8_SRGB,
                                  extent->extent3d(),
                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                  VK_IMAGE_ASPECT_COLOR_BIT)
                    .write_image(Passes::finalImage,
                                 VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                                 VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

void
Cel::Renderer::Passes::tonemap_pass(
    Resource<Asset::SharedAssetData>& shared,
    ParallelResource<RenderGraph::PassServer>& passServer)
{ /*
     VkCommandBuffer cmd;

     {
         cmd = passServer.write()->get_cmd_buffer(Passes::tonemapPass);
     }

     auto access = passServer.partial_read();

     const static auto pipeline = PipelineBuilder(access->get_device())
                                      .add_graphics_shader("tonemap")
                                      .disable_depth_test()
                                      .build();

     VkRenderingAttachmentInfo colourAttachment = Initialisers::attachment_info(
         access->get_resource(Passes::finalImage).imageView,
         nullptr,
         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

     VkRenderingInfo renderInfo = Initialisers::rendering_info(
         access->get_extent(), &colourAttachment, nullptr);

     vkCmdBeginRendering(cmd, &renderInfo);

     vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

     auto imageIndice =
    ;

    vkCmdPushConstants(cmd,
                       pipeline.pipelineLayout,
                       VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(uint32_t),
                       &imageIndice);

    vkCmdEndRendering(cmd);
    */
}
