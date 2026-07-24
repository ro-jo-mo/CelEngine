#pragma once

#include "RenderGraph.h"
#include "renderer/VulkanTypes.h"
#include "renderer/VulkanUtils.h"

#include <string>

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
    explicit PassBuilder(RenderGraph& graph,
                         VkDevice device,
                         VmaAllocator& allocator)
        : graph(graph)
        , device(device)
        , allocator(allocator)
    {
    }

    Handle<AllocatedBuffer> create_buffer(const std::string& name,
                                          size_t allocSize,
                                          VkBufferUsageFlags usages,
                                          VmaMemoryUsage memoryUsage) const;
    Handle<AllocatedImage> create_image(const std::string& name,
                                        VkFormat format,
                                        VkExtent3D extent,
                                        VkImageUsageFlags usages,
                                        VkImageAspectFlags aspects) const;

    void read_buffer();
    void read_image();

    void write_buffer();
    void write_image();

  private:
    RenderGraph& graph;
    VkDevice device;
    VmaAllocator& allocator;
};

}