#pragma once
#include "RenderGraphTypes.h"
#include "renderer/VulkanTypes.h"

#include <array>
#include <cstdint>
#include <optional>

namespace Cel::Renderer::RenderGraph {

class SubmitSplitter
{
  public:
    SubmitSplitter(uint32_t maxPasses,
                   const std::array<uint32_t, 16>& queueToIndex,
                   const std::vector<uint32_t>& passToOrder,
                   const std::vector<uint32_t>& passToQueueOrder);

    struct SubmitSplit
    {
        uint32_t start;
        uint32_t end;
        uint32_t queue;
    };

void add_release_and_acquire(Handle<RenderPass> releasing,
                                 Handle<RenderPass> acquiring,

                                 uint32_t releaseQueue,
                                 uint32_t acquireQueue);

    std::vector<SubmitSplit> compile(
        const std::array<size_t, QUEUE_COUNT>& submitSizes);

private:
    struct Pass
    {
        std::array<uint32_t, QUEUE_COUNT> dependsOn{ UINT32_MAX,
                                                     UINT32_MAX,
                                                     UINT32_MAX };
    };


    void branch_queue();

// Indexed by passToQueueOrder[pass.id]
    std::array<std::vector<std::optional<Pass>>, QUEUE_COUNT> passes;

    const std::array<uint32_t, 16>& queueToIndex;
    const std::vector<uint32_t>& passToOrder;
    const std::vector<uint32_t>& passToQueueOrder;
};
}