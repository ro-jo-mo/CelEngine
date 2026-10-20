#pragma once
#include "RenderGraphTypes.h"
#include "renderer/RenderTypes.h"

#include <array>
#include <cstdint>
#include <optional>

namespace Cel::Renderer::RenderGraph {

// All usage of "queue" inside this class is a value mapped 0..QUEUE_COUNT, not actual queue family values
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
// In the this case, the queue value is already mapped to 0..QUEUE_COUNT
        uint32_t queue;
    };

    void add_release_and_acquire(Handle<RenderPass> releasing,
                                 Handle<RenderPass> acquiring,

                                 uint32_t releaseQueue,
                                 uint32_t acquireQueue);

    std::vector<SubmitSplit> compile(
        const std::array<size_t, QUEUE_COUNT>& submitSizes);

    const std::vector<uint32_t>& passToOrder;
    const std::vector<uint32_t>& passToQueueOrder;

  private:
    struct Pass
    {
        std::array<uint32_t, QUEUE_COUNT> dependsOn{ UINT32_MAX,
                                                     UINT32_MAX,
                                                     UINT32_MAX };
    };

    bool branch_queue(uint32_t targetSubmit,
                      uint32_t currentQueue,
                      std::array<uint32_t, QUEUE_COUNT>& _upTo,
                      std::array<std::array<bool, QUEUE_COUNT>, QUEUE_COUNT>&
                          _isAcquiringFrom,
                      std::vector<SubmitSplit>& out,
                      const std::array<size_t, QUEUE_COUNT>& submitSizes);

    // Indexed by passToQueueOrder[pass.id]
    std::array<std::vector<std::optional<Pass>>, QUEUE_COUNT> passes;

    const std::array<uint32_t, 16>& queueToIndex;
};
}