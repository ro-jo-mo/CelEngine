#include "renderer/render-graph/PassBuilder.h"

#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"

using namespace Cel::Renderer;

PassBuilder&
PassBuilder::create_buffer(const std::string& bufferName,
                           const size_t allocSize,
                           VkBufferUsageFlags usages,
                           VmaMemoryUsage memoryUsage)
{
}

PassBuilder&
PassBuilder::create_image(const std::string& imageName,
                          const VkFormat format,
                          const VkExtent3D extent,
                          const VkImageUsageFlags usages,
                          const VkImageAspectFlags aspects)
{
}

PassBuilder&
PassBuilder::read_buffer(std::string bufferName,
                         VkAccessFlags2 access,
                         VkPipelineStageFlags2 stages,
                         VkDeviceSize offset,
                         VkDeviceSize size)
{
}

PassBuilder&
PassBuilder::read_image(std::string imageName,
                        VkAccessFlags2 access,
                        VkPipelineStageFlags2 stages,
                        VkImageLayout layout)
{
}

PassBuilder&
PassBuilder::write_buffer(std::string bufferName,
                          VkAccessFlags2 access,
                          VkPipelineStageFlags2 stages,
                          VkDeviceSize offset,
                          VkDeviceSize size)
{
}

PassBuilder&
PassBuilder::write_image(std::string imageName,
                         VkAccessFlags2 access,
                         VkPipelineStageFlags2 stages,
                         VkImageLayout layout)
{
}

void
PassBuilder::build(Rendergraph& graph)
{
    VK_IMAGE_DEPTH
    for (const auto& cmd : commands) {
        cmd(graph);
    }
}
