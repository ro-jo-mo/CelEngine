#include "renderer/render-graph/PassBuilder.h"

#include "core/Error.h"
#include "renderer/VulkanHelpers.h"

using namespace Cel::Renderer;
using namespace Cel::Renderer::RenderGraph;

PassBuilder&
PassBuilder::create_buffer(const Handle<AllocatedBuffer> buffer,
                           bool perFrame,
                           size_t allocSize,
                           VkBufferUsageFlags usages,
                           VmaMemoryUsage memoryUsage)
{
    pass.newBuffers.emplace_back(
        buffer, perFrame, BufferRequirements{ allocSize, usages, memoryUsage });

    return *this;
}

PassBuilder&
PassBuilder::create_image(const Handle<AllocatedImage> image,
                          bool perFrame,
                          VkFormat format,
                          VkExtent3D extent,
                          VkImageUsageFlags usages,
                          VkImageAspectFlags aspects)
{
    pass.newImages.emplace_back(
        image, perFrame, ImageRequirements{ format, extent, usages, aspects });

    return *this;
}

PassBuilder&
PassBuilder::read_buffer(const Handle<AllocatedBuffer> buffer,
                         VkPipelineStageFlags2 stages,
                         VkAccessFlags2 flags)
{
    pass.bufferReads.emplace_back(buffer, BufferAccess{ flags, stages });

    return *this;
}

PassBuilder&
PassBuilder::read_image(Handle<AllocatedImage> image,
                        VkPipelineStageFlags2 stages,
                        VkImageLayout layout,
                        VkAccessFlags2 flags)
{
    pass.imageReads.emplace_back(image, ImageAccess{ flags, stages, layout });

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
