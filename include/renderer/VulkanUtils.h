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
load_shader(const char* path, VkDevice device, VkShaderModule* outShaderModule);

void
transition_image_layout(VkCommandBuffer cmd,
                      VkImage image,
                      VkImageLayout currentLayout,
                      VkImageLayout newLayout);

void
transition_image_layout(VkCommandBuffer cmd,
                      VkImage image,
                      VkImageLayout currentLayout,
                      VkImageLayout newLayout,
                      VkImageSubresourceRange subresourceRange);

void
copy_image_to_image(VkCommandBuffer cmd,
                 VkImage source,
                 VkImage destination,
                 VkExtent2D srcSize,
                 VkExtent2D dstSize);

[[nodiscard]] AllocatedImage
create_image(const void* data,
            VkExtent3D size,
            VkFormat format,
            VkImageUsageFlags usage,
            bool mipmapped,

            const char* allocName,
            VulkanContext& context,
            VmaAllocator& allocator,
            const ImmediateSubmit& immediate,
            const GraphicsQueue& graphicsQueue);

[[nodiscard]] AllocatedImage
create_image(VkExtent3D size,
            VkFormat format,
            VkImageUsageFlags usage,
            bool mipmapped,

            const char* allocName,
            VulkanContext& context,
            VmaAllocator& allocator);

[[nodiscard]] AllocatedImage
create_image(VkImageCreateInfo imageCreateInfo,
            VkImageViewCreateInfo imageViewCreateInfo,

            const char* allocName,
            VulkanContext& context,
            VmaAllocator& allocator);

[[nodiscard]] AllocatedImage
create_cube_map(ktxTexture* texture,
              VkFormat format,

              const char* allocName,
              VulkanContext& context,
              VmaAllocator& allocator,
              const ImmediateSubmit& immediate,
              const GraphicsQueue& graphicsQueue);

[[nodiscard]] AllocatedBuffer
create_buffer(size_t allocSize,
             VkBufferUsageFlags usage,
             VmaMemoryUsage memoryUsage,

             const char* allocName,
             const VmaAllocator& allocator);

[[nodiscard]] AllocatedMeshBuffer
upload_mesh(std::vector<uint32_t>& indices,
           std::vector<Vertex>& vertices,
           VulkanContext& context,
           VmaAllocator& allocator,
           ImmediateSubmit& immediate,
           GraphicsQueue& queue);

[[nodiscard]] AllocatedMeshBuffer
upload_mesh(std::vector<uint32_t>& indices,
           std::vector<float>& vertices,

           VulkanContext& context,
           VmaAllocator& allocator,
           ImmediateSubmit& immediate,
           GraphicsQueue& queue);

void
submit_immediate(std::function<void(VkCommandBuffer cmd)>&& function,
                const VulkanContext& context,
                const ImmediateSubmit& immediate,
                const GraphicsQueue& queue);

[[nodiscard]] uint32_t
calculate_mip_map_levels(VkExtent3D extent);

[[nodiscard]] uint32_t
calculate_mip_map_levels(VkExtent2D extent);

void
generate_mip_maps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

void
destroy_buffer(const AllocatedBuffer& buffer, const VmaAllocator& allocator);

void
upload_to_buffer(const void* data,
               uint32_t size,
               VkBuffer destination,
               uint32_t destinationOffset,

               VulkanContext& context,
               VmaAllocator& allocator,
               ImmediateSubmit& immediate,
               GraphicsQueue& queue);
};
