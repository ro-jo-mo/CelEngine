#pragma once

#include "common/Handle.h"

#include <variant>
#include <vector>

namespace Cel::Renderer {

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

    struct PassExecuteCmd
    {
        Handle<RenderPass> pass;
    };

    using Command =
        std::variant<BufferBarrierCmd, ImageBarrierCmd, PassExecuteCmd>;

    ExecutionPlan branch_off();

    ExecutionPlan* original;

    std::vector<Command> plan;
};

}