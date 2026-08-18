#include "renderer/render-graph/RenderGraph.h"

#include "renderer/render-graph/ExecutionPlan.h"
#include "renderer/resource-management/ResourceTracker.h"

#include <map>
#include <ranges>

using namespace Cel::Renderer;

Cel::Common::RelativeScheduler<Cel::Handle<RenderPass>,
                               Cel::Common::Scheduler<Cel::Handle<RenderPass>>>
RenderGraph::add_pass(const RenderPass& pass)
{
    passes.insert({ pass.id, pass });
    return add_system(pass.id);
}

void
RenderGraph::compile()
{
    // What do I need to do?
    // When deciding between two passes, for now I will just brute force. Open
    // up a new branch to search. Once finished, compare total cost
    // For each branch we must keep track of resource state
    // When states are incompatible with what a pass needs, a barrier must be
    // inserted
    // Occasionally a semaphore is also needed when a resource transitions
    // between queues

    // Someone needs to check when a resource is last used, so it can be reused

    compile_passes();

    Resources::BranchingResourceTracker tracker =
        resourceManager.branch_tracker();

    search_branch(iter, tracker);
}

void
RenderGraph::compile_passes()
{

    for (auto& pass : passes | std::views::values) {
        for (auto& create : pass.newBuffers) {
            bufferHandleToMapped[create.id] =
                resourceManager.get_handle_from_requirements(
                    create.requirements);
        }
    }
}

void
RenderGraph::search_branch(PassGraph::Iterator iter,
                           Resources::BranchingResourceTracker& tracker)
{
    while (true) {
        const auto& nodes = iter.get_best_nodes();

        // Have we finished?
        if (nodes.empty()) {
            break;
        }

        // No need to branch if it's linear
        if (nodes.size() == 1) {
        }

        // Create a new branch for each pass
        for (const auto& handle : nodes) {
            auto branchIter = iter.branch_off();
        }

        // Barriers? Create resources?
        // Aliasing? Add to plan?
        // Based on current resource tracker state, decide whether we need
        // barrier? If so update tracker state
        // Tracker likely also needs to
        // tag resource as written (until barrier is inserted flushing it?)
        // Perhaps its simplest, that after creating an execution plan, it
        // is then compiled, and barriers are merged? To do this, the plan
        // would need "false barriers" that mark a resource as needing to
        // stay in its current state
        // As well as something marking a resource as dirty?

        // Ideally we would merge many barriers into the pipeline barrier
        // cmd

        search_branch();
    }
}

void
RenderGraph::add_pass(Handle<RenderPass> handle,
                      ExecutionPlan& plan,
                      PassGraph::Iterator& iter,
                      Resources::BranchingResourceTracker& tracker)
{
    iter.mark_finished(handle);

    auto& pass = compiledPasses[handle.index];

    // for each read / write, is it on right queue?

    for (const auto& read : pass.bufferReads) {
        tracker.read.mark(read.id);

        if (is_barrier_needed(read, tracker)) {
        }
    }
}

bool
RenderGraph::is_barrier_needed(const BufferReadC& access,
                               Resources::BranchingResourceTracker& tracker)
{
    if (tracker.dirty.is_marked(access.id)) {
        return true;
    }

    const auto state = tracker.get_state(access.id);

    if (state.queue != access.access.queue) {
        return true;
    }

    return false;
}

bool
RenderGraph::is_barrier_needed(const ImageReadC& access,
                               Resources::BranchingResourceTracker& tracker)
{
    if (tracker.dirty.is_marked(access.id)) {
        return true;
    }

    const auto state = tracker.get_state(access.id);

    if (state.queue != access.access.queue) {
        return true;
    }
    if (state.layout != access.access.layout) {
        return true;
    }

    return false;
}

bool
RenderGraph::is_barrier_needed(const BufferWriteC& access,
                               Resources::BranchingResourceTracker& tracker)
{
    if (tracker.dirty.is_marked(access.inId)) {
        return true;
    }
    if (tracker.read.is_marked(access.inId)) {
        return true;
    }

    const auto state = tracker.get_state(access.inId);

    if (state.queue != access.access.queue) {
        return true;
    }

    return false;
}

bool
RenderGraph::is_barrier_needed(const ImageWriteC& access,
                               Resources::BranchingResourceTracker& tracker)
{
    if (tracker.dirty.is_marked(access.inId)) {
        return true;
    }
    if (tracker.read.is_marked(access.inId)) {
        return true;
    }

    const auto state = tracker.get_state(access.inId);

    if (state.queue != access.access.queue) {
        return true;
    }
    if (state.layout != access.access.layout) {
        return true;
    }

    return false;
}
