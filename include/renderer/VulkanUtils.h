#pragma once

#include "AssetTypes.h"
#include "VulkanTypes.h"
#include "ecs/Resource.h"

#include <functional>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Cel::Renderer::Utils {
bool
LoadShader(const char* path, VkDevice device, VkShaderModule* outShaderModule);

void
TransitionImageLayout(VkCommandBuffer cmd,
                      VkImage image,
                      VkImageLayout current,
                      VkImageLayout next);

void
CopyImageToImage(VkCommandBuffer cmd,
                 VkImage source,
                 VkImage destination,
                 VkExtent2D srcSize,
                 VkExtent2D dstSize);

AllocatedImage
CreateImage(const void* data,
            VkExtent3D size,
            VkFormat format,
            VkImageUsageFlags usage,
            bool mipmapped,
            const char* allocName,
            VulkanContext& context,
            VmaAllocator& allocator,
            const ImmediateSubmit& immediate,
            const GraphicsQueue& graphicsQueue);

AllocatedImage
CreateImage(VkExtent3D size,
            VkFormat format,
            VkImageUsageFlags usage,
            bool mipmapped,
            const char* allocName,
            VulkanContext& context,
            VmaAllocator& allocator);

AllocatedBuffer
CreateBuffer(size_t allocSize,
             VkBufferUsageFlags usage,
             VmaMemoryUsage memoryUsage,
             const char* allocName,
             const VmaAllocator& allocator);

AllocatedMeshBuffer
UploadMesh(std::vector<uint32_t>& indices,
           std::vector<Vertex>& vertices,
           VulkanContext& context,
           VmaAllocator& allocator,
           ImmediateSubmit& immediate,
           GraphicsQueue& queue);

void
SubmitImmediate(std::function<void(VkCommandBuffer cmd)>&& function,
                const VulkanContext& context,
                const ImmediateSubmit& immediate,
                const GraphicsQueue& queue);

void
GenerateMipMaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

void
DestroyBuffer(const AllocatedBuffer& buffer, const VmaAllocator& allocator);
};
