#pragma once

#include "common/Handle.h"

#include <variant>
#include <vector>

namespace Cel::Renderer {

struct RenderPass;

class ExecutionPlan
{
    // Add barriers to list
    // List is flushed when executing the pipeline barrier
    struct BufferBarrierCmd
    {};
    struct ImageBarrierCmd
    {};

    // Actual submission of barrier
    struct PipelineBarrierCmd
    {};

    struct SemaphoreCmd
    {};

    struct TransferCmd
    {};

    struct ReleaseCmd
    {};

    struct SetQueueCmd
    {};

    struct PassExecuteCmd
    {
        Handle<RenderPass> pass;
    };

    using Command =
        std::variant<BufferBarrierCmd, ImageBarrierCmd, PassExecuteCmd>;

    ExecutionPlan branch_off();

    ExecutionPlan* original = nullptr;

    std::vector<Command> plan;
};

}