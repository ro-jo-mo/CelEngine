#pragma once

#include "RenderPass.h"
#include "renderer/VulkanTypes.h"
#include "renderer/VulkanUtils.h"

#include <string>
#include <utility>

namespace Cel::Renderer {

struct RenderPass;

// buffers created by this are created for each frame in flight
// Later I might add single buffers

// Indirection through handles abstracts the per frame nature of resources

// A pass is likely to:
// Set push constants
// Write to buffers
// Write to depth, color attachments

// At a later stage I'd like to improve the memory allocations here
class PassBuilder
{
  public:
    explicit PassBuilder(const Handle<RenderPass> id) { pass.id = id; }

    // Creating resources should simply create a list of requirements for the
    // buffer / image
    // Resources from previous passes that are not used by any future passes and
    // share the same requirements (or a superset) can then be reused
    /**
     * @brief Creates a per frame buffer
     * @param buffer
     * @param name
     * @param allocSize
     * @param usages
     * @param memoryUsage
     */
    PassBuilder& create_buffer(Handle<AllocatedBuffer> buffer,
                               size_t allocSize,
                               VkBufferUsageFlags usages,
                               VmaMemoryUsage memoryUsage);

    PassBuilder& create_image(Handle<AllocatedImage> image,
                              VkFormat format,
                              VkExtent3D extent,
                              VkImageUsageFlags usages,
                              VkImageAspectFlags aspects);

    // To correctly introduce barriers we must know:
    // - Access flags
    // - Pipeline stage flags
    // Optionally for buffers (size + offset)
    // Images require desired image layout
    // We might be able to derive image aspect ?

    /**
     *
     * @param buffer
     * @param bufferName
     * @param access
     * @param stages
     * @param offset Offset into buffer to read.
     * @param size Size of the buffer region your reading. Default 0 represents
     * a read of the entire buffer.
     */
    PassBuilder& read_buffer(Handle<AllocatedBuffer> buffer,
                             VkPipelineStageFlags2 stages,
                             VkDeviceSize offset = 0,
                             VkDeviceSize size = 0);

    PassBuilder& read_image(Handle<AllocatedImage> image,
                            VkPipelineStageFlags2 stages,
                            VkImageLayout layout);

    PassBuilder& write_buffer(Handle<AllocatedBuffer> buffer,
                              VkAccessFlags2 access,
                              VkPipelineStageFlags2 stages,
                              VkDeviceSize offset = 0,
                              VkDeviceSize size = 0);

    PassBuilder& write_image(Handle<AllocatedImage> image,
                             VkAccessFlags2 access,
                             VkPipelineStageFlags2 stages,
                             VkImageLayout layout);

    PassBuilder& set_queue(uint32_t _queue);

    RenderPass build();

  private:
    RenderPass pass;

    friend class RenderGraph;
};

}