#include "renderer/render-graph/ExecutionPlan.h"

using namespace Cel::Renderer;
using namespace Cel::Renderer::RenderGraph;

ExecutionPlan
ExecutionPlan::branch_off()
{
    return ExecutionPlan(this);
}

void
ExecutionPlan::push(const ExecutePass& exec)
{
    execution.push_back(exec);

    // Rough cost estimate
    totalCost +=
        (exec.bufferTransfers.size() + exec.imageTransfers.size()) * 2 +
        exec.bufferBarriers.size() + exec.imageBarriers.size();
}

uint32_t
ExecutionPlan::cost() const
{
    return totalCost;
}

std::vector<ExecutionPlan::ExecutePass>
ExecutionPlan::compile()
{
    std::vector<ExecutePass> compiled;

    // Append in order
    add_execution_to_list(this, compiled);

    return compiled;
}

void
ExecutionPlan::add_execution_to_list(ExecutionPlan* plan,
                                     std::vector<ExecutePass>& list)
{
    if (plan->original != nullptr) {
        add_execution_to_list(plan->original, list);
    }
    list.insert(list.end(), plan->execution.begin(), plan->execution.end());
}
