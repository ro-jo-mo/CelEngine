#pragma once
#include "../VulkanTypes.h"
#include "../VulkanUtils.h"
#include "common/Handle.h"

namespace Cel::Renderer {
class VulkanResourceManager;
}
namespace Cel::Renderer {

/**
 * @brief A large gpu buffer storing a collection of objects unknown size
 * For now the allocation method is very simple, no memory reuse
 * Typically used for vertice/index buffer
 */
class MegaBuffer
{
  public:
    explicit MegaBuffer(BufferRequirements req,
                        const std::string& name,
                        VulkanResourceManager& manager,
                        uint32_t alignment = 0);

    /**
     * @brief Allocate a block in the mega buffer. Data is uploaded to the gpu
     * at the "upload" call
     * @param data a pointer to the data
     * @param size the total size of the data (bytes), to be uploaded
     * @param alignment the alignment requirements of the data
     * @return Pointer to where this data begins in the buffer
     */
    uint32_t allocate(const void* data, uint32_t size);

    void cleanup(const VmaAllocator& allocator) const;

    /**
     * Check how much data we need to upload. If zero, we needn't do anything.
     * Used to size the staging buffer.
     * @return
     */
    uint32_t upload_size() const;

    /**
     * Upload data to the gpu
     * @param cmd
     * @param staging
     */
    void upload(VkCommandBuffer cmd, const AllocatedBuffer& staging);

    Handle<AllocatedBuffer> handle;
    AllocatedBuffer& buffer;

  private:
    std::vector<std::byte> dataToUpload;

    uint32_t currentUsage = 0;
    uint32_t alignment = 0;

    // Flag as max int to show we haven't set it yet
    uint32_t uploadOffset = UINT32_MAX;
};

}