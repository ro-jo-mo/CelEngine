#pragma once

#include "AssetTypes.h"
#include "VulkanTypes.h"
#include "ecs/Resource.h"

#include <functional>
#include <ktx.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace Cel::Renderer::Utils {
bool
LoadShader(const char* path, VkDevice device, VkShaderModule* outShaderModule);

void
TransitionImageLayout(VkCommandBuffer cmd,
                      VkImage image,
                      VkImageLayout currentLayout,
                      VkImageLayout newLayout);

void
TransitionImageLayout(VkCommandBuffer cmd,
                      VkImage image,
                      VkImageLayout currentLayout,
                      VkImageLayout newLayout,
                      VkImageSubresourceRange subresourceRange);

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

AllocatedImage
CreateImage(VkImageCreateInfo imageCreateInfo,
            VkImageViewCreateInfo imageViewCreateInfo,

            const char* allocName,
            VulkanContext& context,
            VmaAllocator& allocator);

AllocatedImage
CreateCubeMap(ktxTexture* texture,
              VkFormat format,

              const char* allocName,
              VulkanContext& context,
              VmaAllocator& allocator,
              const ImmediateSubmit& immediate,
              const GraphicsQueue& graphicsQueue);

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

AllocatedMeshBuffer
UploadMesh(std::vector<uint32_t>& indices,
           std::vector<float>& vertices,
           VulkanContext& context,
           VmaAllocator& allocator,
           ImmediateSubmit& immediate,
           GraphicsQueue& queue);

void
SubmitImmediate(std::function<void(VkCommandBuffer cmd)>&& function,
                const VulkanContext& context,
                const ImmediateSubmit& immediate,
                const GraphicsQueue& queue);

uint32_t
CalculateMipMapLevels(VkExtent3D extent);

uint32_t
CalculateMipMapLevels(VkExtent2D extent);

void
GenerateMipMaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

void
DestroyBuffer(const AllocatedBuffer& buffer, const VmaAllocator& allocator);
};
