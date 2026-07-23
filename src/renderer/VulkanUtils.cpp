#include "renderer/VulkanUtils.h"

#include "core/Error.h"
#include "renderer/VulkanHelpers.h"

#include <fstream>
#include <ranges>
#include <vector>

bool
Cel::Renderer::Utils::load_shader(const char* path,
                                  VkDevice device,
                                  VkShaderModule* outShaderModule)
{
    // open the file. With cursor at the end
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    // find what the size of the file is by looking up the location of the
    // cursor because the cursor is at the end, it gives the size directly in
    // bytes
    size_t fileSize = (size_t)file.tellg();

    // spirv expects the buffer to be on uint32, so make sure to reserve a int
    // vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // put file cursor at beginning
    file.seekg(0);

    // load the entire file into the buffer
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    // now that the file is loaded into the buffer, we can close it
    file.close();

    return load_shader(buffer.data(),
                       buffer.size() * sizeof(uint32_t),
                       device,
                       outShaderModule);
}

bool
Cel::Renderer::Utils::load_shader(const uint32_t* data,
                                  const size_t size,
                                  VkDevice device,
                                  VkShaderModule* outShaderModule)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.codeSize = size;
    createInfo.pCode = data;

    // check that the creation goes well.
    VkShaderModule shaderModule;

    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
        VK_SUCCESS) {

        return false;
    }

    *outShaderModule = shaderModule;

    return true;
}

void
Cel::Renderer::Utils::transition_image_layout(VkCommandBuffer cmd,
                                              VkImage image,
                                              VkImageLayout currentLayout,
                                              VkImageLayout newLayout)
{
    VkImageMemoryBarrier2 imageBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2
    };
    imageBarrier.pNext = nullptr;
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask =
        VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    VkImageAspectFlags aspectMask =
        (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
            ? VK_IMAGE_ASPECT_DEPTH_BIT
            : VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange =
        Initialisers::image_subresource_range(aspectMask);
    imageBarrier.image = image;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void
Cel::Renderer::Utils::transition_image_layout(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout currentLayout,
    VkImageLayout newLayout,
    VkImageSubresourceRange subresourceRange)
{
    VkImageMemoryBarrier2 imageBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2
    };
    imageBarrier.pNext = nullptr;
    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask =
        VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    imageBarrier.subresourceRange = subresourceRange;
    imageBarrier.image = image;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void
Cel::Renderer::Utils::copy_image_to_image(VkCommandBuffer cmd,
                                          VkImage source,
                                          VkImage destination,
                                          VkExtent2D srcSize,
                                          VkExtent2D dstSize)
{
    VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                             .pNext = nullptr };

    blitRegion.srcOffsets[1].x = srcSize.width;
    blitRegion.srcOffsets[1].y = srcSize.height;
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = dstSize.width;
    blitRegion.dstOffsets[1].y = dstSize.height;
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                               .pNext = nullptr };

    blitInfo.dstImage = destination;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage = source;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    vkCmdBlitImage2(cmd, &blitInfo);
}

Cel::Renderer::AllocatedImage
Cel::Renderer::Utils::create_image(const void* data,
                                   VkExtent3D size,
                                   VkFormat format,
                                   VkImageUsageFlags usage,
                                   bool mipmapped,

                                   const char* allocName,
                                   VulkanContext& context,
                                   VmaAllocator& allocator,
                                   const ImmediateSubmit& immediate,
                                   const GraphicsQueue& graphicsQueue)
{

    size_t dataSize = size.depth * size.width * size.height * 4;
    AllocatedBuffer uploadBuffer =
        create_buffer(dataSize,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VMA_MEMORY_USAGE_CPU_TO_GPU,
                      "image_upload_buffer_alloc",
                      allocator);

    memcpy(uploadBuffer.info.pMappedData, data, dataSize);

    AllocatedImage newImage =
        create_image(size,
                     format,
                     usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                     mipmapped,
                     allocName,
                     context,
                     allocator);

    submit_immediate(
        [&](VkCommandBuffer cmd) {
            Utils::transition_image_layout(
                cmd,
                newImage.image,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkBufferImageCopy copyRegion = {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;

            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = size;

            // copy the buffer into the image
            vkCmdCopyBufferToImage(cmd,
                                   uploadBuffer.buffer,
                                   newImage.image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1,
                                   &copyRegion);

            if (mipmapped) {
                Utils::generate_mip_maps(
                    cmd,
                    newImage.image,
                    VkExtent2D{ newImage.imageExtent.width,
                                newImage.imageExtent.height });
            } else {
                Utils::transition_image_layout(
                    cmd,
                    newImage.image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }
        },
        context,
        immediate,
        graphicsQueue);
    destroy_buffer(uploadBuffer, allocator);

    return newImage;
}

Cel::Renderer::AllocatedImage
Cel::Renderer::Utils::create_image(VkExtent3D size,
                                   VkFormat format,
                                   VkImageUsageFlags usage,
                                   bool mipmapped,

                                   const char* allocName,
                                   VulkanContext& context,
                                   VmaAllocator& allocator)
{
    AllocatedImage newImage;
    newImage.imageFormat = format;
    newImage.imageExtent = size;

    VkImageCreateInfo imageCreateInfo =
        Initialisers::image_create_info(format, usage, size);

    if (mipmapped) {
        imageCreateInfo.mipLevels = calculate_mip_map_levels(size);
    }

    // always allocate images on dedicated GPU memory
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags =
        static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    vk_check(vmaCreateImage(allocator,
                            &imageCreateInfo,
                            &allocInfo,
                            &newImage.image,
                            &newImage.allocation,
                            nullptr));

    // if the format is a depth format, we will need to have it use the correct
    // aspect flag
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT) {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // build a image-view for the image
    VkImageViewCreateInfo view_info = Initialisers::image_view_create_info(
        format, newImage.image, aspectFlag);
    view_info.subresourceRange.levelCount = imageCreateInfo.mipLevels;

    vk_check(vkCreateImageView(
        context.device, &view_info, nullptr, &newImage.imageView));

    vmaSetAllocationName(allocator, newImage.allocation, allocName);

    return newImage;
}

Cel::Renderer::AllocatedImage
Cel::Renderer::Utils::create_image(VkImageCreateInfo imageCreateInfo,
                                   VkImageViewCreateInfo imageViewCreateInfo,

                                   const char* allocName,
                                   VulkanContext& context,
                                   VmaAllocator& allocator)
{
    AllocatedImage newImage;
    newImage.imageFormat = imageCreateInfo.format;
    newImage.imageExtent = imageCreateInfo.extent;

    // always allocate images on dedicated GPU memory
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.requiredFlags =
        static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    vk_check(vmaCreateImage(allocator,
                            &imageCreateInfo,
                            &allocInfo,
                            &newImage.image,
                            &newImage.allocation,
                            nullptr));

    // if the format is a depth format, we will need to have it use the correct
    // aspect flag
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (newImage.imageFormat == VK_FORMAT_D32_SFLOAT) {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    imageViewCreateInfo.image = newImage.image;

    vk_check(vkCreateImageView(
        context.device, &imageViewCreateInfo, nullptr, &newImage.imageView));

    vmaSetAllocationName(allocator, newImage.allocation, allocName);

    return newImage;
}

Cel::Renderer::AllocatedImage
Cel::Renderer::Utils::create_cube_map(ktxTexture* texture,
                                      VkFormat format,

                                      const char* allocName,
                                      VulkanContext& context,
                                      VmaAllocator& allocator,
                                      const ImmediateSubmit& immediate,
                                      const GraphicsQueue& graphicsQueue)
{
    // Read basic image data from ktx file
    VkExtent3D extent{ .width = texture->baseWidth,
                       .height = texture->baseHeight,
                       .depth = 1 };
    uint32_t mipLevels = texture->numLevels;
    ktx_uint8_t* textureData = ktxTexture_GetData(texture);
    ktx_size_t textureSize = ktxTexture_GetDataSize(texture);

    // Create upload buffer
    AllocatedBuffer uploadBuffer =
        create_buffer(textureSize,
                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VMA_MEMORY_USAGE_CPU_TO_GPU,
                      "skybox_upload_buffer_alloc",
                      allocator);
    // Upload image into buffer
    memcpy(uploadBuffer.info.pMappedData, textureData, textureSize);

    // Create cubemap image on gpu
    VkImageCreateInfo imageCreateInfo = Initialisers::image_create_info(
        format,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        extent);
    imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageCreateInfo.arrayLayers = 6;
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkImageViewCreateInfo imageViewCreateInfo =
        Initialisers::image_view_create_info(
            format, VK_NULL_HANDLE, VK_IMAGE_ASPECT_COLOR_BIT);
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.subresourceRange = {
        VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 6
    };

    AllocatedImage newImage = create_image(
        imageCreateInfo, imageViewCreateInfo, allocName, context, allocator);

    // Move image data from buffer to gpu image
    submit_immediate(
        [&](VkCommandBuffer cmd) {
            std::vector<VkBufferImageCopy> copyRegions;
            copyRegions.reserve(6 * mipLevels);

            for (size_t face = 0; face < 6; face++) {
                for (size_t mip = 0; mip < mipLevels; mip++) {
                    ktx_size_t offset;
                    auto err = ktxTexture_GetImageOffset(
                        texture, mip, 0, face, &offset);
                    assert(err == KTX_SUCCESS);

                    VkBufferImageCopy copy{};
                    copy.bufferOffset = offset;

                    copy.imageExtent.width = extent.width >> mip;
                    copy.imageExtent.height = extent.height >> mip;
                    copy.imageExtent.depth = 1;

                    copy.imageSubresource.aspectMask =
                        VK_IMAGE_ASPECT_COLOR_BIT;
                    copy.imageSubresource.mipLevel = mip;
                    copy.imageSubresource.baseArrayLayer = face;
                    copy.imageSubresource.layerCount = 1;

                    copyRegions.push_back(copy);
                }
            }

            VkImageSubresourceRange subresourceRange = {};
            subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresourceRange.baseMipLevel = 0;
            subresourceRange.levelCount = mipLevels;
            subresourceRange.layerCount = 6;

            transition_image_layout(cmd,
                                    newImage.image,
                                    VK_IMAGE_LAYOUT_UNDEFINED,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // copy the buffer into the image
            vkCmdCopyBufferToImage(cmd,
                                   uploadBuffer.buffer,
                                   newImage.image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   copyRegions.size(),
                                   copyRegions.data());

            transition_image_layout(cmd,
                                    newImage.image,
                                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    subresourceRange);
        },
        context,
        immediate,
        graphicsQueue);

    destroy_buffer(uploadBuffer, allocator);

    return newImage;
}

Cel::Renderer::AllocatedBuffer
Cel::Renderer::Utils::create_buffer(const size_t allocSize,
                                    const VkBufferUsageFlags usage,
                                    const VmaMemoryUsage memoryUsage,
                                    const char* allocName,
                                    const VmaAllocator& allocator)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size = allocSize;

    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaAllocInfo = {};
    vmaAllocInfo.usage = memoryUsage;
    vmaAllocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    AllocatedBuffer newBuffer{};

    vk_check(vmaCreateBuffer(allocator,
                             &bufferInfo,
                             &vmaAllocInfo,
                             &newBuffer.buffer,
                             &newBuffer.allocation,
                             &newBuffer.info));

    vmaSetAllocationName(allocator, newBuffer.allocation, allocName);

    return newBuffer;
}

void
Cel::Renderer::Utils::submit_immediate(
    std::function<void(VkCommandBuffer cmd)>&& function,
    const VulkanContext& context,
    const ImmediateSubmit& immediate,
    const GraphicsQueue& queue)
{
    vk_check(vkResetFences(context.device, 1, &immediate.fence));
    vk_check(vkResetCommandBuffer(immediate.commandBuffer, 0));

    auto beginInfo = Initialisers::command_buffer_begin_info(
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    vkBeginCommandBuffer(immediate.commandBuffer, &beginInfo);
    function(immediate.commandBuffer);
    vkEndCommandBuffer(immediate.commandBuffer);

    VkCommandBufferSubmitInfo submitInfo =
        Initialisers::command_buffer_submit_info(immediate.commandBuffer);
    VkSubmitInfo2 submitInfo2 =
        Initialisers::submit_info(&submitInfo, nullptr, nullptr);

    vk_check(vkQueueSubmit2(queue.queue, 1, &submitInfo2, immediate.fence));
    vk_check(vkWaitForFences(
        context.device, 1, &immediate.fence, VK_TRUE, UINT64_MAX));
}

uint32_t
Cel::Renderer::Utils::calculate_mip_map_levels(VkExtent3D extent)
{
    return static_cast<uint32_t>(
               std::floor(std::log2(std::max(extent.width, extent.height)))) +
           1;
}

uint32_t
Cel::Renderer::Utils::calculate_mip_map_levels(VkExtent2D extent)
{
    return static_cast<uint32_t>(
               std::floor(std::log2(std::max(extent.width, extent.height)))) +
           1;
}

void
Cel::Renderer::Utils::generate_mip_maps(VkCommandBuffer cmd,
                                        VkImage image,
                                        VkExtent2D imageSize)
{
    auto mipLevels = calculate_mip_map_levels(imageSize);

    for (uint32_t mip = 0; mip < mipLevels; mip++) {

        VkExtent2D halfSize = imageSize;
        halfSize.width /= 2;
        halfSize.height /= 2;

        VkImageMemoryBarrier2 imageBarrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .pNext = nullptr
        };

        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask =
            VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange =
            Initialisers::image_subresource_range(aspectMask);
        imageBarrier.subresourceRange.levelCount = 1;
        imageBarrier.subresourceRange.baseMipLevel = mip;
        imageBarrier.image = image;

        VkDependencyInfo depInfo{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                  .pNext = nullptr };
        depInfo.imageMemoryBarrierCount = 1;
        depInfo.pImageMemoryBarriers = &imageBarrier;

        vkCmdPipelineBarrier2(cmd, &depInfo);

        if (mip < mipLevels - 1) {
            VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
                                     .pNext = nullptr };

            blitRegion.srcOffsets[1].x = imageSize.width;
            blitRegion.srcOffsets[1].y = imageSize.height;
            blitRegion.srcOffsets[1].z = 1;

            blitRegion.dstOffsets[1].x = halfSize.width;
            blitRegion.dstOffsets[1].y = halfSize.height;
            blitRegion.dstOffsets[1].z = 1;

            blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.srcSubresource.baseArrayLayer = 0;
            blitRegion.srcSubresource.layerCount = 1;
            blitRegion.srcSubresource.mipLevel = mip;

            blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.dstSubresource.baseArrayLayer = 0;
            blitRegion.dstSubresource.layerCount = 1;
            blitRegion.dstSubresource.mipLevel = mip + 1;

            VkBlitImageInfo2 blitInfo{ .sType =
                                           VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
                                       .pNext = nullptr };
            blitInfo.dstImage = image;
            blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            blitInfo.srcImage = image;
            blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            blitInfo.filter = VK_FILTER_LINEAR;
            blitInfo.regionCount = 1;
            blitInfo.pRegions = &blitRegion;

            vkCmdBlitImage2(cmd, &blitInfo);

            imageSize = halfSize;
        }
    }

    // transition all mip levels into the final read_only layout
    transition_image_layout(cmd,
                            image,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void
Cel::Renderer::Utils::destroy_buffer(const AllocatedBuffer& buffer,
                                     const VmaAllocator& allocator)
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

Cel::Renderer::AllocatedMeshBuffer
Cel::Renderer::Utils::upload_mesh(std::vector<uint32_t>& indices,
                                  std::vector<Vertex>& vertices,
                                  VulkanContext& context,
                                  VmaAllocator& allocator,
                                  ImmediateSubmit& immediate,
                                  GraphicsQueue& queue)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(Vertex);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    AllocatedMeshBuffer newSurface;
    newSurface.indexCount = indices.size();

    newSurface.vertexBuffer = create_buffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        "vertex_buffer_alloc",
        allocator);

    VkBufferDeviceAddressInfo deviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = newSurface.vertexBuffer.buffer
    };
    newSurface.vertexBufferAddress =
        vkGetBufferDeviceAddress(context.device, &deviceAddressInfo);

    newSurface.indexBuffer = create_buffer(indexBufferSize,
                                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                           VMA_MEMORY_USAGE_GPU_ONLY,
                                           "index_buffer_alloc",
                                           allocator);

    AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VMA_MEMORY_USAGE_CPU_ONLY,
                                            "mesh_staging_buffer_alloc",
                                            allocator);

    void* data = staging.info.pMappedData;

    // copy vertex buffer
    memcpy(data, vertices.data(), vertexBufferSize);
    // copy index buffer
    memcpy(static_cast<char*>(data) + vertexBufferSize,
           indices.data(),
           indexBufferSize);

    submit_immediate(
        [&](VkCommandBuffer cmd) {
            VkBufferCopy vertexCopy{ 0 };
            vertexCopy.dstOffset = 0;
            vertexCopy.srcOffset = 0;
            vertexCopy.size = vertexBufferSize;

            vkCmdCopyBuffer(cmd,
                            staging.buffer,
                            newSurface.vertexBuffer.buffer,
                            1,
                            &vertexCopy);

            VkBufferCopy indexCopy{ 0 };
            indexCopy.dstOffset = 0;
            indexCopy.srcOffset = vertexBufferSize;
            indexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(cmd,
                            staging.buffer,
                            newSurface.indexBuffer.buffer,
                            1,
                            &indexCopy);
        },
        context,
        immediate,
        queue);

    destroy_buffer(staging, allocator);

    return newSurface;
}

Cel::Renderer::AllocatedMeshBuffer
Cel::Renderer::Utils::upload_mesh(std::vector<uint32_t>& indices,
                                  std::vector<float>& vertices,
                                  VulkanContext& context,
                                  VmaAllocator& allocator,
                                  ImmediateSubmit& immediate,
                                  GraphicsQueue& queue)
{
    const size_t vertexBufferSize = vertices.size() * sizeof(float);
    const size_t indexBufferSize = indices.size() * sizeof(uint32_t);

    AllocatedMeshBuffer newSurface;
    newSurface.indexCount = indices.size();

    newSurface.vertexBuffer = create_buffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        "vertex_buffer_alloc",
        allocator);

    VkBufferDeviceAddressInfo deviceAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = newSurface.vertexBuffer.buffer
    };
    newSurface.vertexBufferAddress =
        vkGetBufferDeviceAddress(context.device, &deviceAddressInfo);

    newSurface.indexBuffer = create_buffer(indexBufferSize,
                                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                           VMA_MEMORY_USAGE_GPU_ONLY,
                                           "index_buffer_alloc",
                                           allocator);

    AllocatedBuffer staging = create_buffer(vertexBufferSize + indexBufferSize,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VMA_MEMORY_USAGE_CPU_ONLY,
                                            "mesh_staging_buffer_alloc",
                                            allocator);

    void* data = staging.info.pMappedData;

    // copy vertex buffer
    memcpy(data, vertices.data(), vertexBufferSize);
    // copy index buffer
    memcpy(static_cast<char*>(data) + vertexBufferSize,
           indices.data(),
           indexBufferSize);

    submit_immediate(
        [&](VkCommandBuffer cmd) {
            VkBufferCopy vertexCopy{};
            vertexCopy.dstOffset = 0;
            vertexCopy.srcOffset = 0;
            vertexCopy.size = vertexBufferSize;

            vkCmdCopyBuffer(cmd,
                            staging.buffer,
                            newSurface.vertexBuffer.buffer,
                            1,
                            &vertexCopy);

            VkBufferCopy indexCopy{ 0 };
            indexCopy.dstOffset = 0;
            indexCopy.srcOffset = vertexBufferSize;
            indexCopy.size = indexBufferSize;

            vkCmdCopyBuffer(cmd,
                            staging.buffer,
                            newSurface.indexBuffer.buffer,
                            1,
                            &indexCopy);
        },
        context,
        immediate,
        queue);

    destroy_buffer(staging, allocator);

    return newSurface;
}
void
Cel::Renderer::Utils::upload_to_buffer(const void* data,
                                       const uint32_t size,
                                       VkBuffer destination,
                                       const uint32_t destinationOffset,
                                       VulkanContext& context,
                                       VmaAllocator& allocator,
                                       ImmediateSubmit& immediate,
                                       GraphicsQueue& queue)
{
    AllocatedBuffer staging = create_buffer(size,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            VMA_MEMORY_USAGE_CPU_ONLY,
                                            "upload_to_XX_staging_buffer_alloc",
                                            allocator);

    memcpy(staging.info.pMappedData, data, size);

    submit_immediate(
        [=](VkCommandBuffer cmd) {
            VkBufferCopy copy{};
            copy.dstOffset = destinationOffset;
            copy.srcOffset = 0;
            copy.size = size;

            vkCmdCopyBuffer(cmd, staging.buffer, destination, 1, &copy);
        },
        context,
        immediate,
        queue);

    destroy_buffer(staging, allocator);
}
