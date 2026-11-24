#pragma once

#include "../components/RenderTypes.h"
#include "common/Handle.h"

#include <string>

namespace Cel::Renderer {
namespace RenderGraph {
class PassServer;
class PassBuilder;
}
class VulkanResourceManager;
}
namespace Cel::Renderer {

/**
 * @brief A large gpu buffer storing a collection of objects unknown size
 * For now the allocation method is very simple, no memory reuse
 * Typically used for vertex/index buffer
 */
class MegaBuffer
{
  public:
    explicit MegaBuffer(const BufferRequirements& req,
                        const std::string& name,
                        VulkanResourceManager& manager,
                        uint32_t alignment = 0);

    /**
     * @brief Allocate a block in the mega buffer. Data is uploaded to the gpu
     * at the "upload" call
     * @param data a pointer to the data
     * @param size the total size of the data (bytes), to be uploaded
     * @return Pointer to where this data begins in the buffer
     */
    uint32_t allocate(const void* data, uint32_t size);

    /**
     * Upload data to the gpu
     * @param cmd
     * @param staging
     * @param server
     */
    void record_upload_cmd(VkCommandBuffer cmd,
                           Handle<AllocatedBuffer> staging,
                           const RenderGraph::PassServer& server);

    void register_upload_to_pass(Handle<AllocatedBuffer> stagingHandle,
                                 RenderGraph::PassBuilder& pass) const;

    Handle<AllocatedBuffer> handle;
    AllocatedBuffer buffer;

  private:
    std::vector<std::byte> dataToUpload;

    uint32_t currentUsage = 0;
    uint32_t alignment = 0;

    uint32_t totalSize;

    // Flag as max int to show we haven't set it yet
    uint32_t uploadOffset = UINT32_MAX;
};

}