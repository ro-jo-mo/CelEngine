#pragma once

#include "../resource-management/MegaBuffer.h"
#include "RenderGraphTypes.h"
#include "common/Handle.h"

#include <vector>

namespace Cel::Renderer::RenderGraph {

struct RenderPass;

class ExecutionPlan
{
  public:
    ExecutionPlan() = default;

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

    void push(const ExecutePass& exec);

    [[nodiscard]] uint32_t cost() const;

    std::vector<ExecutePass> compile();

  private:
    explicit ExecutionPlan(ExecutionPlan* original)
        : original(original)
        , totalCost(original->totalCost)
    {
    }

    static void add_execution_to_list(ExecutionPlan* plan,
                                      std::vector<ExecutePass>& list);

    ExecutionPlan* original = nullptr;

    uint32_t totalCost = 0;

    // An ordered representation of the plan
    std::vector<ExecutePass> execution;
};

}