#include "renderer/render-graph/PassBuilder.h"

#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"

using namespace Cel::Renderer;

PassBuilder&
PassBuilder::create_buffer(const std::string& name,
                           size_t allocSize,
                           VkBufferUsageFlags usages,
                           VmaMemoryUsage memoryUsage)
{
    pass.newBuffers.emplace_back(name, allocSize, usages, memoryUsage);

    return *this;
}

PassBuilder&
PassBuilder::create_image(const std::string& name,
                          VkFormat format,
                          VkExtent3D extent,
                          VkImageUsageFlags usages,
                          VkImageAspectFlags aspects)
{
    pass.newImages.emplace_back(name, format, extent, usages, aspects);

    return *this;
}

PassBuilder&
PassBuilder::read_buffer(const std::string& bufferName,
                         VkAccessFlags2 access,
                         VkPipelineStageFlags2 stages,
                         VkDeviceSize offset,
                         VkDeviceSize size)
{
    pass.bufferReads.emplace_back(bufferName, access, stages, offset, size);

    return *this;
}

PassBuilder&
PassBuilder::read_image(const std::string& imageName,
                        VkAccessFlags2 access,
                        VkPipelineStageFlags2 stages,
                        VkImageLayout layout)
{
    pass.imageReads.emplace_back(imageName, access, stages, layout);

    return *this;
}
PassBuilder&
PassBuilder::write_buffer(const std::string& bufferName,
                          const std::string& outName,
                          VkAccessFlags2 access,
                          VkPipelineStageFlags2 stages,
                          VkDeviceSize offset,
                          VkDeviceSize size)
{
    pass.bufferWrites.emplace_back(
        bufferName, outName, access, stages, offset, size);

    return *this;
}

PassBuilder&
PassBuilder::write_image(const std::string& imageName,
                         const std::string& outName,
                         VkAccessFlags2 access,
                         VkPipelineStageFlags2 stages,
                         VkImageLayout layout)
{
    pass.imageWrites.emplace_back(imageName, outName, access, stages, layout);

    return *this;
}

PassBuilder&
PassBuilder::set_execute(const std::function<void(void*)>& execute)
{
    pass.execute = execute;

    return *this;
}

RenderPass
PassBuilder::build()
{
    // Basic validity checks

    return pass;
}
