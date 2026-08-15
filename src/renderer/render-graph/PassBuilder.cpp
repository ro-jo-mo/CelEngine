#include "renderer/render-graph/PassBuilder.h"

#include "core/Error.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"

using namespace Cel::Renderer;

PassBuilder&
PassBuilder::create_buffer(const std::string& name,
                           size_t allocSize,
                           VkBufferUsageFlags usages,
                           VmaMemoryUsage memoryUsage)
{
    pass.newBuffers.emplace_back(
        name, BufferRequirements{ allocSize, usages, memoryUsage });

    return *this;
}

PassBuilder&
PassBuilder::create_image(const std::string& name,
                          VkFormat format,
                          VkExtent3D extent,
                          VkImageUsageFlags usages,
                          VkImageAspectFlags aspects)
{
    pass.newImages.emplace_back(
        name, ImageRequirements{ format, extent, usages, aspects });

    return *this;
}

PassBuilder&
PassBuilder::read_buffer(const std::string& bufferName,
                         VkPipelineStageFlags2 stages,
                         VkDeviceSize offset,
                         VkDeviceSize size)
{
    pass.bufferReads.emplace_back(
        bufferName,
        BufferAccess{ VK_ACCESS_2_SHADER_READ_BIT, stages, offset, size });

    return *this;
}

PassBuilder&
PassBuilder::read_image(const std::string& imageName,
                        VkPipelineStageFlags2 stages,
                        VkImageLayout layout)
{
    pass.imageReads.emplace_back(
        imageName, ImageAccess{ VK_ACCESS_2_SHADER_READ_BIT, stages, layout });

    return *this;
}
PassBuilder&
PassBuilder::write_buffer(const std::string& inName,
                          const std::string& outName,
                          VkAccessFlags2 access,
                          VkPipelineStageFlags2 stages,
                          VkDeviceSize offset,
                          VkDeviceSize size)
{
    pass.bufferWrites.emplace_back(
        inName, outName, BufferAccess{ access, stages, offset, size });

    return *this;
}

PassBuilder&
PassBuilder::write_image(const std::string& imageName,
                         const std::string& outName,
                         VkAccessFlags2 access,
                         VkPipelineStageFlags2 stages,
                         VkImageLayout layout)
{
    pass.imageWrites.emplace_back(
        imageName, outName, ImageAccess{ access, stages, layout });

    return *this;
}

PassBuilder&
PassBuilder::set_execute(
    const std::function<void(VkCommandBuffer, void*)>& execute)
{
    pass.execute = execute;

    return *this;
}

PassBuilder&
PassBuilder::set_queue(const uint32_t _queue)
{
    queue = _queue;

    return *this;
}

RenderPass
PassBuilder::build()
{
    // Basic validity checks
    if (pass.execute == nullptr) {
        throw_error("render pass callback function has not been set");
    }
    if (pass.imageWrites.size() + pass.bufferWrites.size() == 0) {
        throw_error("render pass must write to at least one resource");
    }

    auto set_queues = [this](auto& data) {
        for (auto& value : data) {
            value.access.queue = queue;
        }
    };

    set_queues(pass.bufferReads);
    set_queues(pass.bufferWrites);
    set_queues(pass.imageReads);
    set_queues(pass.imageWrites);

    return pass;
}
