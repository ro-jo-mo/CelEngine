#include "renderer/MegaBuffer.h"

uint32_t
Cel::Renderer::MegaBuffer::UploadData(const void* data,
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

    Utils::UploadToBuffer(data,
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
Cel::Renderer::MegaBuffer::Cleanup(const VmaAllocator& allocator)
{
    Utils::DestroyBuffer(buffer, allocator);
}
