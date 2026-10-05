#pragma once

#include "../resource-management/MegaBuffer.h"
#include "RenderGraphTypes.h"
#include "common/Handle.h"

#include <map>
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
        std::array<VkSemaphoreSubmitInfo, QUEUE_COUNT> waitSemaphoreInfo{};
    };

    struct SubmitSplitter
    {
        struct SubmitSplit
        {
            uint32_t start;
            uint32_t end;
            uint32_t queue;
        };

        struct Transfer
        {
            Handle<RenderPass> pass;
            bool isAcquire;
            uint32_t releaseQueue;
            uint32_t acquireQueue;
        };

        // We could later compile consecutive splits on the same queue into a
        // single one
        // Easy to do now actually
        void add_release(Handle<RenderPass> pass,
                         uint32_t passOrder,
                         uint32_t releaseQueue,
                         uint32_t acquireQueue);

        void add_acquire(Handle<RenderPass> pass,
                         uint32_t passOrder,
                         uint32_t releaseQueue,
                         uint32_t acquireQueue);

        std::vector<SubmitSplit> compile(const std::array<Queue, 3>& queues);

        // order -> pass & queue family
        std::map<uint32_t, std::vector<Transfer>> releaseAndAcquires;
    };

    struct SubmitData
    {
        std::array<std::vector<VkSubmitInfo2>, QUEUE_COUNT> submits;
        std::vector<SubmitSplitter::SubmitSplit> splits;
        std::vector<PassSubmitInfo> preBarriers;
        std::vector<PassSubmitInfo> postBarriers;
        std::vector<VkCommandBufferSubmitInfo> recorded;
    };

    static void add_barriers(std::vector<PassSubmitInfo>& preBarriers,
                             const ExecutePass& pass,
                             VulkanResourceManager& manager);

    static void add_transfers(std::vector<PassSubmitInfo>& preBarriers,
                              std::vector<PassSubmitInfo>& postBarriers,
                              const std::vector<uint32_t>& passToOrder,
                              SubmitSplitter& splitter,
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

    static void add_wait_semaphore(PassSubmitInfo& info,
                                   VkSemaphore semaphore,
                                   uint64_t signalValue);

    static void record_barriers(VkCommandBuffer cmd, PassSubmitInfo& info);

    static void add_execution_to_list(ExecutionPlan* plan,
                                      std::vector<ExecutePass>& list);

    static SubmitData create_submit_infos(
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