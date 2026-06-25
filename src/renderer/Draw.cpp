#include "renderer/Draw.h"

#include "renderer/Camera.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"

using namespace Cel;
using namespace Cel::Renderer;

void
DrawData::Draw()
{
    frameData = currentFrameData->Get();
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

    cmd = frameData.commandBuffer;
    VkCommandBufferBeginInfo beginInfo = Initialisers::CommandBufferBeginInfo(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    vkBeginCommandBuffer(cmd, &beginInfo);

    Utils::TransitionImageLayout(cmd,
                                 drawImage->image,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    Utils::TransitionImageLayout(cmd,
                                 depthImage->image,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    // DRAW GEOMETRY HERE

    DrawGeometry();

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

    // RENDERING FINISHED

    // Finally submit the command buffer for execution
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
    }
}
void
DrawData::DrawGeometry()
{
    // Early return if there's no geometry to render

    const uint32_t textureDescriptorCount =
        assetServer->textureCache.descriptors.size();

    if (textureDescriptorCount == 0) {
        return;
    }
    
    // Rendering info setup
    VkRenderingAttachmentInfo colourAttachment =
        Initialisers::AttachmentInfo(drawImage->imageView,
                                     nullptr,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo depthAttachment =
        Initialisers::DepthAttachmentInfo(
            depthImage->imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo renderInfo = Initialisers::RenderingInfo(
        renderExtent->extent, &colourAttachment, &depthAttachment);

    vkCmdBeginRendering(cmd, &renderInfo);

    // Scene setup
    auto sceneBuffer = Utils::CreateBuffer(sizeof(SceneData),
                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           VMA_MEMORY_USAGE_CPU_TO_GPU,
                                           *allocator);
    {
        auto sceneBufferData =
            static_cast<SceneData*>(sceneBuffer.info.pMappedData);
        SceneData data{ .viewMatrix = camera.GetViewMatrix(),
                        .projectionMatrix =
                            camera.GetProjectionMatrix(swapchain->extent) };
        data.viewProjMatrix = data.projectionMatrix * data.viewMatrix;
        *sceneBufferData = data;
    }

    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo;
    allocArrayInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    allocArrayInfo.pNext = nullptr;
    allocArrayInfo.descriptorSetCount = 1;
    allocArrayInfo.pDescriptorCounts = &textureDescriptorCount;

    VkDescriptorSet sceneDescriptor = frameData.descriptorAllocator.Allocate(
        globalDescriptors->sceneLayout, &allocArrayInfo);

    DescriptorWriter writer;
    writer.WriteBuffer(0,
                       sceneBuffer.buffer,
                       sizeof(SceneData),
                       0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    VkWriteDescriptorSet arraySet;
    arraySet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    arraySet.descriptorCount = textureDescriptorCount;
    arraySet.dstArrayElement = 0;
    arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    arraySet.pImageInfo = assetServer->textureCache.descriptors.data();

    writer.Write(arraySet);
    fmt::println("here");
    writer.UpdateSet(context->device, sceneDescriptor);

    fmt::println("there");
    // Bind pipeline, scene descriptor, scissor etc
    BindSceneData(sceneDescriptor);

    for (const auto& [transform, mesh, material] : renderables) {
        DrawModel(transform, mesh, material);
    }

    vkCmdEndRendering(cmd);

    frameData.toDelete.Push([=, allocator = *allocator]() {
        vmaDestroyBuffer(allocator, sceneBuffer.buffer, sceneBuffer.allocation);
    });
}

void
DrawData::BindSceneData(VkDescriptorSet sceneDescriptor) const
{
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline->layout,
                            0,
                            1,
                            &sceneDescriptor,
                            0,
                            nullptr);

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(renderExtent->extent.width);
    viewport.height = static_cast<float>(renderExtent->extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = renderExtent->extent;

    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void
DrawData::DrawModel(GlobalTransform& transform,
                    Handle<Mesh> meshHandle,
                    Handle<Material> matHandle) const
{
    // Get stuff from asset server
    Material material = assetServer->GetMaterial(matHandle);
    AllocatedMeshBuffer mesh = assetServer->GetMesh(meshHandle);

    // Bind current material descriptor
    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline->layout,
                            1,
                            1,
                            &material.materialSet,
                            0,
                            nullptr);

    // Bind current index buffer
    vkCmdBindIndexBuffer(cmd, mesh.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Upload push constants
    EntityPushConstants pushConstants{ .transform = transform.transform,
                                       .vertexBuffer =
                                           mesh.vertexBufferAddress };

    vkCmdPushConstants(cmd,
                       pipeline->layout,
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0,
                       sizeof(EntityPushConstants),
                       &pushConstants);

    vkCmdDrawIndexed(cmd, mesh.indexCount, 1, 0, 0, 0);
}

void
Renderer::Draw(
    Query<With<GlobalTransform, Handle<Mesh>, Handle<Material>>>& renderables,
    Query<With<Camera>>& cameras,
    Resource<VulkanContext>& context,
    Resource<Swapchain>& swapchain,
    Resource<GraphicsQueue>& graphicsQueue,
    Resource<DrawImage>& drawImage,
    Resource<DepthImage>& depthImage,
    Resource<MeshPipeline>& pipeline,
    Resource<RenderExtent>& renderExtent,
    Resource<CurrentFrameData>& currentFrameData,
    Resource<AssetServer>& assetServer,
    Resource<GlobalDescriptorData>& globalDescriptors,
    Resource<VmaAllocator>& allocator)
{
    if (cameras.begin() == cameras.end()) {
        // No camera, no draw
        return;
    }

    auto [cam] = *cameras.begin();
    DrawData data = { renderables,   cameras,
                      context,       swapchain,
                      graphicsQueue, drawImage,
                      depthImage,    pipeline,
                      renderExtent,  currentFrameData,
                      assetServer,   globalDescriptors,
                      allocator,     cam };

    data.Draw();
}
