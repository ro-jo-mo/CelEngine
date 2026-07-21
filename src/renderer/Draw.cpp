#include "renderer/Draw.h"

#include "renderer/Camera.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"

using namespace Cel;
using namespace Cel::Renderer;

void
DrawData::draw()
{
    frame = &frameData->Get();
    vk_check(vkWaitForFences(
        context->device, 1, &frame->renderFence, VK_TRUE, UINT64_MAX));

    cleanup_draw();

    uint32_t swapchainIndex;
    VkResult result = vkAcquireNextImageKHR(context->device,
                                            swapchain->swapchain,
                                            UINT64_MAX,
                                            frame->acquireSemaphore,
                                            VK_NULL_HANDLE,
                                            &swapchainIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Introduce code to handle resizing swapchain
        fmt::println("Out of date swapchain, needs resizing");
        return;
    }

    vk_check(vkResetFences(context->device, 1, &frame->renderFence));
    vk_check(vkResetCommandBuffer(frame->commandBuffer, 0));

    cmd = frame->commandBuffer;
    VkCommandBufferBeginInfo beginInfo = Initialisers::command_buffer_begin_info(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    vkBeginCommandBuffer(cmd, &beginInfo);

    Utils::transition_image_layout(cmd,
                                 drawImage->image,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    Utils::transition_image_layout(cmd,
                                 depthImage->image,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
    // DRAW GEOMETRY HERE
    create_indirect_data();

    draw_geometry();

    draw_skybox();

    vkCmdEndRendering(cmd);

    // DRAWING FINISHED
    Utils::transition_image_layout(cmd,
                                 drawImage->image,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    Utils::transition_image_layout(cmd,
                                 swapchain->images[swapchainIndex],
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    Utils::copy_image_to_image(cmd,
                            drawImage->image,
                            swapchain->images[swapchainIndex],
                            renderExtent->extent,
                            swapchain->extent);

    Utils::transition_image_layout(cmd,
                                 swapchain->images[swapchainIndex],
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    vk_check(vkEndCommandBuffer(cmd));

    // RENDERING FINISHED

    // Finally submit the command buffer for execution
    VkCommandBufferSubmitInfo bufferSubmitInfo =
        Initialisers::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo waitInfo = Initialisers::semaphore_submit_info(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
        frame->acquireSemaphore);

    VkSemaphoreSubmitInfo signalInfo = Initialisers::semaphore_submit_info(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
        swapchain->submitSemaphores[swapchainIndex]);

    VkSubmitInfo2 submitInfo =
        Initialisers::submit_info(&bufferSubmitInfo, &signalInfo, &waitInfo);

    vk_check(vkQueueSubmit2(
        graphicsQueue->queue, 1, &submitInfo, frame->renderFence));

    VkPresentInfoKHR presentInfo = Initialisers::present_info();
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

    frameData->Tick();
}

void
DrawData::cleanup_draw()
{
    frameData->Get().toDelete.flush();
    frameData->Get().descriptorAllocator.clear_pools();
}

void
DrawData::draw_geometry()
{
    // Rendering info setup
    VkRenderingAttachmentInfo colourAttachment =
        Initialisers::attachment_info(drawImage->imageView,
                                     nullptr,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo depthAttachment =
        Initialisers::depth_attachment_info(
            depthImage->imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo renderInfo = Initialisers::rendering_info(
        renderExtent->extent, &colourAttachment, &depthAttachment);

    vkCmdBeginRendering(cmd, &renderInfo);

    // Scene setup
    auto sceneBuffer = Utils::create_buffer(sizeof(SceneData),
                                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                           VMA_MEMORY_USAGE_CPU_TO_GPU,
                                           "scene_buffer_alloc",
                                           *allocator);
    {
        VkBufferDeviceAddressInfo vertInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = assetServer->verticeBuffer.buffer.buffer
        };
        VkBufferDeviceAddressInfo matInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = assetServer->materialBuffer.buffer.buffer
        };
        VkBufferDeviceAddressInfo entityInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = entityBuffer.buffer
        };

        // TODO: add per entity data buffer address
        auto sceneBufferData =
            static_cast<SceneData*>(sceneBuffer.info.pMappedData);
        SceneData data{
            .verticesBufferAddress =
                vkGetBufferDeviceAddress(context->device, &vertInfo),
            .materialBufferAddress =
                vkGetBufferDeviceAddress(context->device, &matInfo),
            .perEntityBufferAddress =
                vkGetBufferDeviceAddress(context->device, &entityInfo),
            .viewMatrix = camera.get_view_matrix(),
            .projectionMatrix = camera.get_projection_matrix(swapchain->extent)
        };

        data.viewProjMatrix = data.projectionMatrix * data.viewMatrix;

        *sceneBufferData = data;
    }

    const uint32_t textureDescriptorCount =
        assetServer->textureCache.descriptors.size();

    VkDescriptorSetVariableDescriptorCountAllocateInfo allocArrayInfo;
    allocArrayInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    allocArrayInfo.pNext = nullptr;
    allocArrayInfo.descriptorSetCount = 1;
    allocArrayInfo.pDescriptorCounts = &textureDescriptorCount;

    VkDescriptorSet sceneDescriptor = frame->descriptorAllocator.allocate(
        globalDescriptors->sceneLayout, &allocArrayInfo);

    DescriptorWriter writer;
    writer.write_buffer(0,
                       sceneBuffer.buffer,
                       sizeof(SceneData),
                       0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    if (textureDescriptorCount > 0) {
        VkWriteDescriptorSet arraySet;
        arraySet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        arraySet.descriptorCount = textureDescriptorCount;
        arraySet.dstArrayElement = 0;
        arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        arraySet.dstBinding = 1;
        arraySet.pImageInfo = assetServer->textureCache.descriptors.data();
        arraySet.pNext = nullptr;

        writer.write(arraySet);
    }

    writer.update_set(context->device, sceneDescriptor);

    // Bind pipeline, scene descriptor, scissor etc
    bind_scene_data(sceneDescriptor);

    // Bind indices
    vkCmdBindIndexBuffer(
        cmd, assetServer->indiceBuffer.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // Lastly execute indirect draw
    vkCmdDrawIndexedIndirect(cmd,
                             indirectBuffer.buffer,
                             0,
                             renderables.size(),
                             sizeof(VkDrawIndexedIndirectCommand));

    frame->toDelete.push(
        [=, a = *allocator, eb = entityBuffer, id = indirectBuffer]() {
            Utils::destroy_buffer(sceneBuffer, a);
            Utils::destroy_buffer(eb, a);
            Utils::destroy_buffer(id, a);
        });
}

void
DrawData::create_indirect_data()
{
    // NOTE: Really these buffers should be built once, and reused each frame
    // This might present problems with multiple frames in flight

    // Create buffer for PerEntityGpuData and indirect calls
    entityBuffer =
        Utils::create_buffer(sizeof(PerEntityGpuData) * renderables.size(),
                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                            VMA_MEMORY_USAGE_GPU_ONLY,
                            "per_entity_data_buffer_alloc",
                            *allocator);
    indirectBuffer = Utils::create_buffer(
        sizeof(VkDrawIndexedIndirectCommand) * renderables.size(),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        "indirect_calls_buffer_alloc",
        *allocator);

    // Accumulate data in lists before loading into buffers
    std::vector<PerEntityGpuData> entityData;
    std::vector<VkDrawIndexedIndirectCommand> indirectCalls;

    entityData.reserve(renderables.size());
    indirectCalls.reserve(renderables.size());

    for (const auto& [transform, meshHandle, matHandle] : renderables) {
        auto mesh = assetServer->get_mesh(meshHandle);
        auto mat = assetServer->get_material(matHandle);

        VkDrawIndexedIndirectCommand call{ .indexCount = mesh.indexCount,
                                           .instanceCount = 1,
                                           .firstIndex = mesh.firstIndex,
                                           .vertexOffset = mesh.vertexOffset,
                                           .firstInstance = 0 };

        indirectCalls.push_back(call);

        PerEntityGpuData data{ .transform = transform.transform,
                               .normalTransform =
                                   glm::mat3(transform.transform),
                               .materialIndex = mat.bufferIndex };

        entityData.push_back(data);
    }

    // copy into buffer
    // We want all this data to be gpu local
    // We could simply create a large staging buffer and reuse it for each
    // upload, which might perform better
    // Alternatively an upload queue might be
    // beneficial as well
    Utils::upload_to_buffer(entityData.data(),
                          sizeof(PerEntityGpuData) * entityData.size(),
                          entityBuffer.buffer,
                          0,
                          *context,
                          *allocator,
                          *immediate,
                          *graphicsQueue);

    Utils::upload_to_buffer(indirectCalls.data(),
                          sizeof(VkDrawIndexedIndirectCommand) *
                              indirectCalls.size(),
                          indirectBuffer.buffer,
                          0,
                          *context,
                          *allocator,
                          *immediate,
                          *graphicsQueue);
}

void
DrawData::draw_skybox()
{
    vkCmdBindPipeline(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline->pipeline);

    auto& cube = assetServer->skyboxCube;

    // Create view buffer for our skybox
    // Remove the translation from our view transform and combine with
    // projection
    auto viewProjBuffer =
        Utils::create_buffer(sizeof(glm::mat4),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_MEMORY_USAGE_CPU_TO_GPU,
                            "skybox_viewproj_buffer_alloc",
                            *allocator);
    {
        auto sceneBufferData =
            static_cast<glm::mat4*>(viewProjBuffer.info.pMappedData);
        // Strip out translation by converting to 3x3 and back
        glm::mat4 viewProj = camera.get_projection_matrix(swapchain->extent) *
                             glm::mat4(glm::mat3(camera.get_view_matrix()));

        *sceneBufferData = viewProj;
    }

    // Write descriptors
    DescriptorWriter writer{};

    // Write view buffer
    writer.write_buffer(0,
                       viewProjBuffer.buffer,
                       sizeof(glm::mat4),
                       0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    // Combine texture image sampler
    auto skyTex =
        assetServer->textureCache.descriptors[assetServer->skyboxTextureIndex];
    writer.write_image(1,
                      skyTex.imageView,
                      skyTex.sampler,
                      skyTex.imageLayout,
                      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    VkDescriptorSet skyboxDescriptor = frame->descriptorAllocator.allocate(
        globalDescriptors->skyboxLayout, nullptr);

    writer.update_set(context->device, skyboxDescriptor);

    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            skyboxPipeline->layout,
                            0,
                            1,
                            &skyboxDescriptor,
                            0,
                            nullptr);

    // Bind cube
    const VkDeviceSize offsets[1] = { 0 };
    vkCmdBindIndexBuffer(cmd, cube.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindVertexBuffers(cmd, 0, 1, &cube.vertexBuffer.buffer, offsets);

    // Lastly draw the skybox
    vkCmdDrawIndexed(cmd, cube.indexCount, 1, 0, 0, 0);

    frame->toDelete.push([=, allocator = *allocator]() {
        vmaDestroyBuffer(
            allocator, viewProjBuffer.buffer, viewProjBuffer.allocation);
    });
}

void
DrawData::bind_scene_data(VkDescriptorSet sceneDescriptor) const
{
    vkCmdBindPipeline(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline->pipeline);
    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            meshPipeline->layout,
                            0,
                            1,
                            &sceneDescriptor,
                            0,
                            nullptr);

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = static_cast<float>(renderExtent->extent.height);
    viewport.width = static_cast<float>(renderExtent->extent.width);
    viewport.height = -static_cast<float>(renderExtent->extent.height);
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
Renderer::draw(
    Query<With<GlobalTransform, Handle<Mesh>, Handle<Material>>>& renderables,
    Query<With<Camera>>& cameras,
    Resource<VulkanContext>& context,
    Resource<Swapchain>& swapchain,
    Resource<GraphicsQueue>& graphicsQueue,
    Resource<DrawImage>& drawImage,
    Resource<DepthImage>& depthImage,
    Resource<MeshPipeline>& meshPipeline,
    Resource<SkyboxPipeline>& skyboxPipeline,
    Resource<RenderExtent>& renderExtent,
    Resource<FrameData>& frameData,
    Resource<AssetServer>& assetServer,
    Resource<GlobalDescriptorData>& globalDescriptors,
    Resource<VmaAllocator>& allocator,
    Resource<ImmediateSubmit>& immediate)
{
    if (cameras.begin() == cameras.end()) {
        // No camera, no draw
        return;
    }

    auto [cam] = *cameras.begin();
    DrawData data = { renderables,       cameras,      context,    swapchain,
                      graphicsQueue,     drawImage,    depthImage, meshPipeline,
                      skyboxPipeline,    renderExtent, frameData,  assetServer,
                      globalDescriptors, allocator,    immediate,  cam };

    data.draw();
}
