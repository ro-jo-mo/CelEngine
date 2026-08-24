#pragma once

#include "../resource-management/MegaBuffer.h"
#include "RenderGraphTypes.h"

#include <unordered_set>

namespace Cel::Renderer::RenderGraph {

// Allows render passes access to things like buffers, images and command
// buffers
class PassServer
{
  public:
    /**
     * @brief Get the cmd buffer for this render pass for recording
     * If null, it means we've effectively culled the pass and recording is
     * unnecessary
     * @param handle
     * @return
     */
    VkCommandBuffer get_cmd_buffer(Handle<RenderPass> handle) const;

  private:
    std::unordered_set<Handle<RenderPass>> passesInUse;

    // Pool per queue per frame in flight per thread
    // Indexed through:
    // + current_frame * (queue_count * thread_count)
    // + current_queue * (thread_count)
    // + current_thread
    // We can create some mapping of render pass to queue family
    std::vector<VkCommandPool> pools;

    std::unordered_map<std::pair<uint32_t, Handle<RenderPass>>, VkCommandBuffer>
        passCmdBuffers;
};

}