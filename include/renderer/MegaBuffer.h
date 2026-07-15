#pragma once
#include "VulkanTypes.h"
#include "VulkanUtils.h"

namespace Cel::Renderer {

/**
 * @brief A large gpu buffer storing a collection of objects unknown size
 * For now the allocation method is very simple, no memory reuse
 * Typically used for vertice/index buffer
 */
class MegaBuffer
{
  public:
    explicit MegaBuffer(const uint32_t size,
                        VkBufferUsageFlags flags,
                        VmaMemoryUsage usage,
                        const char* allocName,
                        const VmaAllocator& allocator)
        : buffer(Utils::CreateBuffer(size, flags, usage, allocName, allocator))
        , maxUsage(size)
    {
    }

    /**
     * @brief Allocate a block in the mega buffer and upload the data to it
     * @param data a pointer to the data
     * @param size the total size of the data (bytes), to be uploaded
     * @param alignment the alignment requirements of the data
     * @return Pointer to where this data begins in the buffer
     */
    uint32_t UploadData(const void* data,
                        uint32_t size,
                        uint32_t alignment,
                        VulkanContext& context,
                        VmaAllocator& allocator,
                        ImmediateSubmit& immediate,
                        GraphicsQueue& queue);
    void Cleanup(const VmaAllocator& allocator);

    AllocatedBuffer buffer;

  private:
    uint32_t currentUsage = 0;
    uint32_t maxUsage;
};

}