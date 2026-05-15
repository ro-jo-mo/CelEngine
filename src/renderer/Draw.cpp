#include "renderer/Draw.h"

#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"

using namespace Cel::Renderer;
using namespace Cel;

void
DrawGeometry(Resource<DrawImage>& drawImage,
             Resource<RenderExtent>& renderExtent,
             VkCommandBuffer& cmd,
             Resource<MeshPipeline>& pipeline)
{
    VkRenderingAttachmentInfo colourAttachment =
        Initialisers::AttachmentInfo(drawImage->imageView,
                                     nullptr,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo = Initialisers::RenderingInfo(
        renderExtent->extent, &colourAttachment, nullptr);

    vkCmdBeginRendering(cmd, &renderInfo);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = renderExtent->extent.width;
    viewport.height = renderExtent->extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;

    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdDraw(cmd, 3, 1, 0, 0);

    vkCmdEndRendering(cmd);
}

void
Cel::Renderer::Draw::Run(Resource<VulkanContext>& context,
                         Resource<Swapchain>& swapchain,
                         Resource<GraphicsQueue>& graphicsQueue,
                         Resource<DrawImage>& drawImage,
                         Resource<MeshPipeline>& pipeline,
                         Resource<RenderExtent>& renderExtent,
                         Resource<CurrentFrameData>& currentFrameData)
{
    auto frameData = currentFrameData->Get();
    VkCheck(vkWaitForFences(
        context->device, 1, &frameData.renderFence, VK_TRUE, UINT64_MAX));

    uint32_t swapchainIndex;
    VkResult result = vkAcquireNextImageKHR(context->device,
                                            swapchain->swapchain,
                                            UINT64_MAX,
                                            frameData.acquireSemaphore,
                                            VK_NULL_HANDLE,
                                            &swapchainIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Introduce code to handle resizing swapchain
        fmt::println("Out of date swapchain, needs resizing");
        return;
    }

    VkCheck(vkResetFences(context->device, 1, &frameData.renderFence));
    VkCheck(vkResetCommandBuffer(frameData.commandBuffer, 0));

    auto cmd = frameData.commandBuffer;
    VkCommandBufferBeginInfo beginInfo = Initialisers::CommandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    vkBeginCommandBuffer(cmd, &beginInfo);

    Utils::TransitionImageLayout(cmd,
                                 drawImage->image,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    // DRAW GEOMETRY HERE

    DrawGeometry(drawImage, renderExtent, cmd, pipeline);

    // DRAWING FINISHED
    Utils::TransitionImageLayout(cmd,
                                 drawImage->image,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    Utils::TransitionImageLayout(cmd,
                                 swapchain->images[swapchainIndex],
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    Utils::CopyImageToImage(cmd,
                            drawImage->image,
                            swapchain->images[swapchainIndex],
                            renderExtent->extent,
                            swapchain->extent);

    Utils::TransitionImageLayout(cmd,
                                 swapchain->images[swapchainIndex],
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VkCheck(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo bufferSubmitInfo =
        Initialisers::CommandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = Initialisers::SemaphoreSubmitInfo(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
        frameData.acquireSemaphore);

    VkSemaphoreSubmitInfo signalInfo = Initialisers::SemaphoreSubmitInfo(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
        swapchain->submitSemaphores[swapchainIndex]);

    VkSubmitInfo2 submitInfo =
        Initialisers::SubmitInfo(&bufferSubmitInfo, &signalInfo, &waitInfo);

    VkCheck(vkQueueSubmit2(
        graphicsQueue->queue, 1, &submitInfo, frameData.renderFence));

    VkPresentInfoKHR presentInfo = Initialisers::PresentInfo();
    presentInfo.pSwapchains = &swapchain->swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &swapchain->submitSemaphores[swapchainIndex];
    presentInfo.pImageIndices = &swapchainIndex;

    VkResult presentResult =
        vkQueuePresentKHR(graphicsQueue->queue, &presentInfo);

    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
        fmt::println("Out of date swapchain, needs resizing");
        return;
    }

    currentFrameData->Update();
}

void
Cel::Renderer::SetRenderExtent::Run(Resource<RenderExtent>& renderExtent,
                                    Resource<DrawImage>& drawImage,
                                    Resource<Swapchain>& swapchain)
{
    renderExtent->extent.height =
        std::min(swapchain->extent.height, drawImage->imageExtent.height) *
        renderExtent->renderScale;
    renderExtent->extent.width =
        std::min(swapchain->extent.width, drawImage->imageExtent.width) *
        renderExtent->renderScale;
}
