#pragma once

#include "RenderGraph.h"
#include "renderer/VulkanTypes.h"
#include "renderer/VulkanUtils.h"

#include <string>
#include <utility>

namespace Cel::Renderer {

// buffers created by this are created for each frame in flight
// Later I might add single buffers

// Indirection through handles abstracts the per frame nature of resources

// A pass is likely to:
// Set push constants
// Write to buffers
// Write to depth, color attachments

class PassBuilder
{
  public:
    explicit PassBuilder(std::string name)
        : name(std::move(name))
    {
    }

    // Creating resources should simply create a list of requirements for the
    // buffer / image
    // Resources from previous passes that are not used by any future passes and
    // share the same requirements (or a superset) can then be reused
    /**
     * @brief Creates a per frame buffer
     * @param name
     * @param allocSize
     * @param usages
     * @param memoryUsage
     */
    PassBuilder& create_buffer(const std::string& name,
                               size_t allocSize,
                               VkBufferUsageFlags usages,
                               VmaMemoryUsage memoryUsage);

    PassBuilder& create_image(const std::string& name,
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
     * @param bufferName
     * @param access
     * @param stages
     * @param offset Offset into buffer to read.
     * @param size Size of the buffer region your reading. Default 0 represents
     * a read of the entire buffer.
     */
    PassBuilder& read_buffer(std::string bufferName,
                             VkAccessFlags2 access,
                             VkPipelineStageFlags2 stages,
                             VkDeviceSize offset = 0,
                             VkDeviceSize size = 0);

    PassBuilder& read_image(std::string imageName,
                            VkAccessFlags2 access,
                            VkPipelineStageFlags2 stages,
                            VkImageLayout layout);

    // I don't think writes need any different args compared to reads
    // Barriers can be placed between writes & reads
    // I don't believe a barrier needs to exist between a write and a write?
    // Honestly two successive writes without a read does not make much sense.
    // Might as well just discard the first result
    // This can go further to say creating an image resource without writing to
    // it does not make sense
    // Two consecutive reads might require a barrier if the format is different
    // Similarly if you read a specific segment of a buffer, and read another
    // segment that is not a subset of the prior read, a barrier will require
    // insertion
    // Writing to a resource will create an output for the pass.
    // The original handle will become invalid after this pass and only
    // accessible under the new name
    PassBuilder& write_buffer(std::string bufferName,
                              std::string outName,
                              VkAccessFlags2 access,
                              VkPipelineStageFlags2 stages,
                              VkDeviceSize offset = 0,
                              VkDeviceSize size = 0);

    PassBuilder& write_image(std::string imageName,
                             std::string outName,
                             VkAccessFlags2 access,
                             VkPipelineStageFlags2 stages,
                             VkImageLayout layout);

    void build(RenderGraph& graph);

  private:
    std::string name;

    std::vector<BufferRequirements> newBuffers;
    std::vector<ImageRequirements> newImages;

    std::vector<BufferRead> bufferReads;
    std::vector<ImageRead> imageReads;

    std::vector<BufferWrite> bufferWrites;
    std::vector<ImageWrite> imageWrites;

    friend class RenderGraph;
};

}