#pragma once

#include "RenderGraphTypes.h"
#include "renderer/VulkanTypes.h"
#include "renderer/VulkanUtils.h"

#include <string>
#include <utility>

namespace Cel::Renderer::RenderGraph {

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

    /**
     * @brief Creates a *per frame* buffer
     * @param buffer
     * @param allocSize
     * @param usages
     * @param memoryUsage
     */
    PassBuilder& create_buffer(Handle<AllocatedBuffer> buffer,
                               size_t allocSize,
                               VkBufferUsageFlags usages,
                               VmaMemoryUsage memoryUsage);

    /**
     * @brief Creates a *per frame* image
     * Likely less used than create_buffer, as per frame images are rarely
     * needed
     * @param buffer
     * @param allocSize
     * @param usages
     * @param memoryUsage
     */
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
                             VkPipelineStageFlags2 stages);

    PassBuilder& read_image(Handle<AllocatedImage> image,
                            VkPipelineStageFlags2 stages,
                            VkImageLayout layout);

    PassBuilder& write_buffer(Handle<AllocatedBuffer> buffer,
                              VkAccessFlags2 access,
                              VkPipelineStageFlags2 stages);

    PassBuilder& write_image(Handle<AllocatedImage> image,
                             VkAccessFlags2 access,
                             VkPipelineStageFlags2 stages,
                             VkImageLayout layout);

    PassBuilder& set_queue(uint32_t queue);

    RenderPass build();

  private:
    RenderPass pass;

    friend class RenderGraph;
};

}