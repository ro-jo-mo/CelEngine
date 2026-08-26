#pragma once

#include "../resource-management/MegaBuffer.h"
#include "RenderGraphTypes.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <unordered_set>

namespace Cel::Renderer::RenderGraph {

// Gives render passes access to image / buffers and command buffers.
class PassServer
{
  public:
    PassServer(VkDevice device, std::vector<uint32_t>& queues);

    /**
     * @brief Get the cmd buffer for this render pass for recording
     * If null, it means we've effectively culled the pass and recording is
     * unnecessary
     * @param handle
     * @return
     */
    [[nodiscard]] VkCommandBuffer get_cmd_buffer(Handle<RenderPass> handle);

    [[nodiscard]] AllocatedBuffer& get_resource(
        Handle<AllocatedBuffer> handle) const;

    [[nodiscard]] AllocatedImage& get_resource(
        Handle<AllocatedImage> handle) const;

    /**
     * Set the current frame data, what passes are running this frame, the
     * mapping of pass handles to real handles
     */
    void update_frame(
        uint32_t _currentFrame,
        const std::unordered_map<Handle<RenderPass>, uint32_t>& _validPasses,
        const std::unordered_map<Handle<AllocatedBuffer>,
                                 Handle<AllocatedBuffer>>& mappedBufferHandles,
        const std::unordered_map<Handle<AllocatedImage>,
                                 Handle<AllocatedImage>>& mappedImageHandles,
        VulkanResourceManager& manager);

  private:
    uint32_t get_pool_index(Handle<RenderPass> handle);

    void allocate_cmd_buffers(uint32_t index);

    std::unordered_set<Handle<RenderPass>> passesInUse;

    // Pool per queue per frame in flight per thread
    // Indexed through:
    // + current_frame * (queue_count * thread_count)
    // + current_queue * (thread_count)
    // + current_thread
    // We can create some mapping of render pass to queue family
    std::vector<VkCommandPool> commandPools;
    // For each commandPools[i] there is a corresponding list of unused buffers
    std::vector<std::vector<VkCommandBuffer>> availableBuffers;

    // Maps queue indices to 0..queue_count
    // Makes the terrible assumption that we'll never use
    // a queue family index >= 16
    std::array<uint32_t, 16> queueToIndex;

    // A mapping of this current frame + render pass -> recorded cmd buffer
    std::array<std::unordered_map<Handle<RenderPass>, VkCommandBuffer>,
               FRAMES_IN_FLIGHT>
        passCmdBuffers;

    std::unordered_map<Handle<AllocatedBuffer>,
                       std::reference_wrapper<AllocatedBuffer>>
        mappedBuffers;
    std::unordered_map<Handle<AllocatedImage>,
                       std::reference_wrapper<AllocatedImage>>
        mappedImages;

    std::array<std::vector<Handle<AllocatedBuffer>>, FRAMES_IN_FLIGHT>
        buffersToFree;
    std::array<std::vector<Handle<AllocatedImage>>, FRAMES_IN_FLIGHT>
        imagesToFree;

    uint32_t currentFrame = 0;
    std::unordered_map<Handle<RenderPass>, uint32_t> validPasses;

    VkDevice device;
};

}