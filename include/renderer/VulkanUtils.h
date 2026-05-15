#pragma once
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
}