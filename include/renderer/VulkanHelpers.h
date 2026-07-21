#pragma once
#include <fmt/printf.h>
#include <source_location>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

// Helpers is somewhat nondescript especially with Utils also existing
// For now it contains VkCheck, and Initialisers

namespace Cel::Renderer {
inline void
vk_check(VkResult err,
        std::source_location loc = std::source_location::current())
{
    if (err) {
        fmt::print("Vulkan error at {}:{} - {}",
                   loc.file_name(),
                   loc.line(),
                   string_VkResult(err));
        abort();
    }
}

}

namespace Cel::Renderer::Initialisers {

VkCommandPoolCreateInfo
command_pool_create_info(uint32_t queueFamilyIndex,
                      VkCommandPoolCreateFlags flags = 0);
VkCommandBufferAllocateInfo
command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1);

VkCommandBufferBeginInfo
command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
VkCommandBufferSubmitInfo
command_buffer_submit_info(VkCommandBuffer cmd);

VkFenceCreateInfo
fence_create_info(VkFenceCreateFlags flags = 0);

VkSemaphoreCreateInfo
semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

VkSubmitInfo2
submit_info(VkCommandBufferSubmitInfo* cmd,
           VkSemaphoreSubmitInfo* signalSemaphoreInfo,
           VkSemaphoreSubmitInfo* waitSemaphoreInfo);
VkPresentInfoKHR
present_info();

VkRenderingAttachmentInfo
attachment_info(
    VkImageView view,
    VkClearValue* clear,
    VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingAttachmentInfo
depth_attachment_info(
    VkImageView view,
    VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

VkRenderingInfo
rendering_info(VkExtent2D renderExtent,
              VkRenderingAttachmentInfo* colorAttachment,
              VkRenderingAttachmentInfo* depthAttachment);

VkImageSubresourceRange
image_subresource_range(VkImageAspectFlags aspectMask);

VkSemaphoreSubmitInfo
semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

VkDescriptorSetLayoutBinding
descriptor_set_layout_binding(VkDescriptorType type,
                           VkShaderStageFlags stageFlags,
                           uint32_t binding);
VkDescriptorSetLayoutCreateInfo
descriptor_set_layout_create_info(VkDescriptorSetLayoutBinding* bindings,
                              uint32_t bindingCount);
VkWriteDescriptorSet
write_descriptor_image(VkDescriptorType type,
                     VkDescriptorSet dstSet,
                     VkDescriptorImageInfo* imageInfo,
                     uint32_t binding);
VkWriteDescriptorSet
write_descriptor_buffer(VkDescriptorType type,
                      VkDescriptorSet dstSet,
                      VkDescriptorBufferInfo* bufferInfo,
                      uint32_t binding);
VkDescriptorBufferInfo
buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

VkImageCreateInfo
image_create_info(VkFormat format,
                VkImageUsageFlags usageFlags,
                VkExtent3D extent);
VkImageViewCreateInfo
image_view_create_info(VkFormat format,
                    VkImage image,
                    VkImageAspectFlags aspectFlags);
VkPipelineLayoutCreateInfo
pipeline_layout_create_info();

VkPipelineShaderStageCreateInfo
pipeline_shader_stage_create_info(VkShaderStageFlagBits stage,
                              VkShaderModule shaderModule,
                              const char* entry = "main");
}