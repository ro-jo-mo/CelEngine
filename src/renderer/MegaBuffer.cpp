#include "renderer/MegaBuffer.h"

uint32_t
Cel::Renderer::MegaBuffer::upload_data(const void* data,
                                      const uint32_t size,
                                      const uint32_t alignment,
                                      VulkanContext& context,
                                      VmaAllocator& allocator,
                                      ImmediateSubmit& immediate,
                                      GraphicsQueue& queue)
{
    if (currentUsage % alignment != 0) {
        currentUsage += alignment - (currentUsage % alignment);
    }

    Utils::upload_to_buffer(data,
                          size,
                          buffer.buffer,
                          currentUsage,
                          context,
                          allocator,
                          immediate,
                          queue);

    const uint32_t offset = currentUsage;

    currentUsage += size;

    return offset;
}

void
Cel::Renderer::MegaBuffer::cleanup(const VmaAllocator& allocator)
{
    Utils::destroy_buffer(buffer, allocator);
}
