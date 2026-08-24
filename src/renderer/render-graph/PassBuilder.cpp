#include "renderer/render-graph/PassBuilder.h"

#include "core/Error.h"
#include "renderer/VulkanHelpers.h"

using namespace Cel::Renderer;
using namespace Cel::Renderer::RenderGraph;

PassBuilder&
PassBuilder::create_buffer(const Handle<AllocatedBuffer> buffer,
                           size_t allocSize,
                           VkBufferUsageFlags usages,
                           VmaMemoryUsage memoryUsage)
{
    pass.newBuffers.emplace_back(
        buffer, BufferRequirements{ allocSize, usages, memoryUsage });

    return *this;
}

PassBuilder&
PassBuilder::create_image(const Handle<AllocatedImage> image,
                          VkFormat format,
                          VkExtent3D extent,
                          VkImageUsageFlags usages,
                          VkImageAspectFlags aspects)
{
    pass.newImages.emplace_back(
        image, ImageRequirements{ format, extent, usages, aspects });

    return *this;
}

PassBuilder&
PassBuilder::read_buffer(const Handle<AllocatedBuffer> buffer,
                         VkPipelineStageFlags2 stages)
{
    pass.bufferReads.emplace_back(
        buffer, BufferAccess{ VK_ACCESS_2_SHADER_READ_BIT, stages });

    return *this;
}

PassBuilder&
PassBuilder::read_image(Handle<AllocatedImage> image,
                        VkPipelineStageFlags2 stages,
                        VkImageLayout layout)
{
    pass.imageReads.emplace_back(
        image, ImageAccess{ VK_ACCESS_2_SHADER_READ_BIT, stages, layout });

    return *this;
}
PassBuilder&
PassBuilder::write_buffer(Handle<AllocatedBuffer> buffer,
                          VkAccessFlags2 access,
                          VkPipelineStageFlags2 stages)
{
    pass.bufferWrites.emplace_back(buffer,
                                   BufferAccess{
                                       access,
                                       stages,
                                   });

    return *this;
}

PassBuilder&
PassBuilder::write_image(const Handle<AllocatedImage> image,
                         VkAccessFlags2 access,
                         VkPipelineStageFlags2 stages,
                         VkImageLayout layout)
{
    pass.imageWrites.emplace_back(image, ImageAccess{ access, stages, layout });

    return *this;
}

PassBuilder&
PassBuilder::set_queue(const uint32_t queue)
{
    pass.queue = queue;

    return *this;
}

RenderPass
PassBuilder::build()
{
    // Basic validity checks

    if (pass.imageWrites.size() + pass.bufferWrites.size() == 0) {
        throw_error("render pass must write to at least one resource");
    }

    auto set_queues = [this](auto& data) {
        for (auto& value : data) {
            value.access.queue = pass.queue;
        }
    };

    set_queues(pass.bufferReads);
    set_queues(pass.bufferWrites);
    set_queues(pass.imageReads);
    set_queues(pass.imageWrites);

    return pass;
}
