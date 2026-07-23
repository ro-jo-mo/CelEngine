#pragma once

#include "RenderGraph.h"
#include "renderer/VulkanTypes.h"
#include "renderer/VulkanUtils.h"

#include <string>

namespace Cel::Renderer {

// Create image

struct PassBuffer;
struct PassImage;

class PassBuilder
{
  public:
    explicit PassBuilder(RenderGraph& owner)
        : owner(owner)
    {
    }

    Handle<PassBuffer> create_buffer(std::string name,
                                     size_t allocSize,
                                     VkBufferUsageFlags usage,
                                     VmaMemoryUsage memoryUsage);
    void create_image();

    void read_buffer();
    void read_image();

    void write_buffer();
    void write_image();

  private:
    RenderGraph& owner;
};

}