#pragma once

#include "RenderGraph.h"
#include "renderer/VulkanTypes.h"

#include <string>

namespace Cel::Renderer {

struct PassBuffer;
struct PassImage;

class PassBuilder
{
  public:
    explicit PassBuilder(RenderGraph& owner)
        : owner(owner)
    {
    }

    Handle<PassBuffer> create_buffer(size_t allocSize,
                                     VkBufferUsageFlags usage,
                                     VmaMemoryUsage memoryUsage,
                                     std::string name);
    void create_image();

    void read_buffer();
    void read_image();

    void write_buffer();
    void write_image();

  private:
    RenderGraph& owner;
};

}