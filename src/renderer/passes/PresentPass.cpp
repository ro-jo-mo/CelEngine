#include "renderer/passes/PresentPass.h"

#include "../../../include/renderer/resource-management/Queues.h"
#include "../../../include/renderer/vulkan/VulkanUtils.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/render-graph/RenderGraph.h"

void
Cel::Renderer::Passes::register_present_pass(
    Resource<RenderGraph::Graph>& graph)
{

    auto pass = RenderGraph::PassBuilder(Passes::presentPass)
                    .read_image(Passes::finalImage,
                                VK_ACCESS_2_TRANSFER_READ_BIT,
                                VK_PIPELINE_STAGE_2_BLIT_BIT,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
                    .set_queue(Queues::graphics);

    graph->set_present_pass(pass.build());
}

void
Cel::Renderer::Passes::record_present_pass(
    Resource<Swapchain>& swapchain,
    ParallelResource<RenderGraph::PassServer>& passServer)
{
    VkCommandBuffer cmd;
    {
        cmd = passServer.write()->get_cmd_buffer(Passes::presentPass);
    }

    // The swapchain was out of date, there is no image to copy to. The pass
    // still needs a (empty) command buffer, as the rest of the frame is
    // submitted as normal, and the present submit signals the frame fence
    if (!swapchain->imageAcquired) {
        return;
    }

    auto access = passServer.partial_read();

    const auto& image = access->get_resource(Passes::finalImage);
    const auto target = swapchain->images[swapchain->imageIndex];

    Utils::transition_image_layout(cmd,
                                   target,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    Utils::copy_image_to_image(
        cmd, image.image, target, access->get_extent(), swapchain->extent);

    Utils::transition_image_layout(cmd,
                                   target,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}
