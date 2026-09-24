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

/**
 * @brief Load a shader from a pointer to raw spv data
 * @param data spv data
 * @param size In bytes
 * @param device
 * @param outShaderModule
 * @return
 */
bool
load_shader(const uint32_t* data,
            size_t size,
            VkDevice device,
            VkShaderModule* outShaderModule);

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
create_image(VkExtent3D size,
             VkFormat format,
             VkImageUsageFlags usage,
             bool mipmapped,

             const char* allocName,
             VulkanContext& context,
             const VmaAllocator& allocator);

[[nodiscard]] AllocatedImage
create_image(Handle<AllocatedImage> handle,
             const VkImageCreateInfo& imageCreateInfo,
             VkImageViewCreateInfo& imageViewCreateInfo,

             const char* allocName,
             VkDevice device,
             const VmaAllocator& allocator);

void
upload_image_asset(const void* data,
                   VkCommandBuffer cmd,
                   const AllocatedImage& image,
                   const AllocatedBuffer& staging);

void
upload_image_asset(ktxTexture* texture,
                   VkCommandBuffer cmd,
                   const AllocatedImage& image,
                   const AllocatedBuffer& staging);

size_t
calculate_image_size(VkExtent3D extent, VkFormat format);

[[nodiscard]] AllocatedBuffer
create_buffer(Handle<AllocatedBuffer> handle,
              size_t allocSize,
              VkBufferUsageFlags usage,
              VmaMemoryUsage memoryUsage,

              const char* allocName,
              const VmaAllocator& allocator);

[[nodiscard]] uint32_t
calculate_mip_map_levels(VkExtent3D extent);

[[nodiscard]] uint32_t
calculate_mip_map_levels(VkExtent2D extent);

void
generate_mip_maps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);

void
destroy_buffer(const AllocatedBuffer& buffer, const VmaAllocator& allocator);

void
upload_to_buffer(VkCommandBuffer cmd,
                 const void* data,
                 uint32_t size,
                 const AllocatedBuffer& destination,
                 uint32_t destinationOffset,
                 const AllocatedBuffer& staging);

void
set_scissor_and_viewport(VkCommandBuffer cmd, const VkExtent2D& extent);

};
