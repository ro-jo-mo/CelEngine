#pragma once

#include "common/Handle.h"
#include "renderer/MegaBuffer.h"

#include <vector>

namespace Cel::Renderer {

struct RenderPass;

// Replica of the actual vk struct
// Main thing is that the buffer / image link is replaced with a handle
struct BufferBarrier
{
    VkPipelineStageFlags2 srcStageMask;
    VkAccessFlags2 srcAccessMask;
    VkPipelineStageFlags2 dstStageMask;
    VkAccessFlags2 dstAccessMask;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
    Handle<AllocatedBuffer> buffer;
};
struct ImageBarrier
{
    VkPipelineStageFlags2 srcStageMask;
    VkAccessFlags2 srcAccessMask;
    VkPipelineStageFlags2 dstStageMask;
    VkAccessFlags2 dstAccessMask;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
    Handle<AllocatedImage> image;
};

// Each pass has an implicit signal semaphore, represented by a
// Handle<RenderPass>. If no pass waits on this semaphore, we can happily
// discard it. Otherwise, it needs to be created and signalled
// A barrier needs to be placed on both queues, but it can be derived from
// justis th The full barrier data can be derived from this
struct BufferTransfer
{
    Handle<RenderPass> semaphore;
    BufferBarrier barrier;
};
struct ImageTransfer
{
    Handle<RenderPass> semaphore;
    ImageBarrier barrier;
};

struct BufferMerge
{
    Handle<AllocatedBuffer> handle;
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
};
struct ImageMerge
{
    Handle<AllocatedImage> handle;
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
};

class ExecutionPlan
{
  public:
    struct ExecutePass
    {
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

  private:
    ExecutionPlan* original = nullptr;

    // An ordered representation of the plan
    std::vector<ExecutePass> execution;
};

}