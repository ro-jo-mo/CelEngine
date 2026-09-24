#include "renderer/render-graph/PassBuilder.h"

#include "core/Error.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/passes/HandleAllocator.h"

using namespace Cel::Renderer;
using namespace Cel::Renderer::RenderGraph;

PassBuilder::PassBuilder(const Handle<RenderPass> id)
{
    pass.id = id;
    pass.queue = UINT32_MAX; // Flag unset
}

PassBuilder&
PassBuilder::create_buffer(const Handle<AllocatedBuffer> buffer,
                           bool perFrame,
                           size_t allocSize,
                           VkBufferUsageFlags2 usages,
                           VmaMemoryUsage memoryUsage)
{
    pass.bufferCreates.emplace_back(
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
    pass.imageCreates.emplace_back(
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
PassBuilder::upload_buffer(Handle<AllocatedBuffer> staging,
                           Handle<AllocatedBuffer> uploadTo)
{
    // We write to the cpu and then read for the transfer
    write_buffer(staging,
                 VK_ACCESS_2_HOST_WRITE_BIT | VK_ACCESS_2_TRANSFER_READ_BIT,
                 VK_PIPELINE_STAGE_2_HOST_BIT | VK_PIPELINE_STAGE_2_COPY_BIT);

    write_buffer(
        uploadTo, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_COPY_BIT);

    return *this;
}

PassBuilder&
PassBuilder::upload_image(Handle<AllocatedBuffer> staging,
                          Handle<AllocatedImage> uploadTo)
{

    return *this;
}

PassBuilder&
PassBuilder::set_queue(const Queue& queue)
{
    pass.queue = queue.family;

    return *this;
}

RenderPass
PassBuilder::build()
{
    // Basic validity checks
    
    if (pass.queue == UINT32_MAX) {
        // I think I'll actually set the queue in build as a parameter instead
        throw_error("Pass: {} does not have a queue set",
                    Passes::HandleAllocator::get_name(pass.id));
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
