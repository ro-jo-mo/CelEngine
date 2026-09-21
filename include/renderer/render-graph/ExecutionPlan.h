#pragma once

#include "../resource-management/MegaBuffer.h"
#include "RenderGraphTypes.h"
#include "common/Handle.h"

#include <vector>

namespace Cel::Renderer::RenderGraph {
class PassServer;

struct RenderPass;

class ExecutionPlan
{
  public:
    ExecutionPlan() = default;

    struct ExecutePass
    {
        Handle<RenderPass> pass;
        // Places where a semaphore needs insertion
        std::vector<BufferTransfer> bufferTransfers;
        std::vector<ImageTransfer> imageTransfers;

        // Barriers to insert prior to the pass
        std::vector<BufferBarrier> bufferBarriers;
        std::vector<ImageBarrier> imageBarriers;

        // Where we can merge a barrier
        std::vector<BufferMerge> bufferMerges;
        std::vector<ImageMerge> imageMerges;
    };

    ExecutionPlan branch_off();

    void push(const ExecutePass& exec);

    [[nodiscard]] uint32_t cost() const;

    std::vector<ExecutePass> compile();

    static void execute(
        std::vector<ExecutePass>& plan,
        const std::unordered_map<Handle<RenderPass>, RenderPass>& passes,
        Handle<RenderPass> presentPass,
        PassServer& passServer,
        Swapchain& swapchain,
        VulkanResourceManager& manager);

  private:
    explicit ExecutionPlan(ExecutionPlan* original)
        : original(original)
        , totalCost(original->totalCost)
    {
    }

    struct PassSubmitInfo
    {
        std::vector<VkBufferMemoryBarrier2> buffers;
        std::vector<VkImageMemoryBarrier2> images;
        VkSemaphoreSubmitInfo signalSemaphoreInfo;
        std::vector<VkSemaphoreSubmitInfo> waitSemaphoreInfo;
    };

    static void add_barriers(std::vector<PassSubmitInfo>& preBarriers,
                             const ExecutePass& pass,
                             VulkanResourceManager& manager);

    static void add_transfers(std::vector<PassSubmitInfo>& preBarriers,
                              std::vector<PassSubmitInfo>& postBarriers,
                              const std::vector<uint32_t>& passToOrder,
                              const ExecutePass& pass,
                              PassServer& passServer,
                              VulkanResourceManager& manager);

    static void add_merges(
        const ExecutePass& pass,
        std::vector<PassSubmitInfo>& preBarriers,
        const std::unordered_map<Handle<AllocatedBuffer>, Handle<RenderPass>>&
            bufferMergePoints,
        const std::unordered_map<Handle<AllocatedImage>, Handle<RenderPass>>&
            imageMergePoints,
        VulkanResourceManager& manager);

    static void record_barriers(VkCommandBuffer cmd, PassSubmitInfo& info);

    static void add_execution_to_list(ExecutionPlan* plan,
                                      std::vector<ExecutePass>& list);

    static void create_submit_infos(
        std::array<std::vector<VkSubmitInfo2>, QUEUE_COUNT>& submits,

        std::vector<ExecutePass>& plan,
        const std::unordered_map<Handle<RenderPass>, RenderPass>& passes,
        PassServer& passServer,
        VulkanResourceManager& manager);

    ExecutionPlan* original = nullptr;

    uint32_t totalCost = 0;

    // An ordered representation of the plan
    std::vector<ExecutePass> execution;
};

}