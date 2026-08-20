#include "renderer/render-graph/RenderGraph.h"

#include "renderer/render-graph/ExecutionPlan.h"
#include "renderer/resource-management/ResourceTracker.h"

#include <map>
#include <ranges>

using namespace Cel::Renderer;

Cel::Common::RelativeScheduler<Cel::Handle<RenderPass>,
                               Cel::Common::Graph<Cel::Handle<RenderPass>>>
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

    auto iter = graph.iter();

    search_branch(iter, tracker);
}

void
RenderGraph::compile_passes()
{
    auto create_helper = [&](auto& creates, auto& addTo) {
        for (auto& create : creates) {
            addTo[create.id] = resourceManager.get_handle_from_requirements(
                create.requirements);
        }
    };

    // We firstly need to create the actual mapping from handle to mapped handle
    for (auto& pass : passes | std::views::values) {
        create_helper(pass.newBuffers, bufferHandleToMapped);
        create_helper(pass.newImages, imageHandleToMapped);
    }

    auto to_mapped = [&](auto& iter, auto& map) {
        for (auto& it : iter) {
            it.id = map[it.id];
        }
    };
    // Then we can update the reads / writes to use the mapped handles
    for (auto& pass : passes | std::views::values) {
        to_mapped(pass.bufferReads, bufferHandleToMapped);
        to_mapped(pass.imageReads, imageHandleToMapped);
        to_mapped(pass.bufferWrites, bufferHandleToMapped);
        to_mapped(pass.imageWrites, imageHandleToMapped);
    }
}

void
RenderGraph::search_branch(Common::Graph<Handle<RenderPass>>::Iterator iter,
                           Resources::BranchingResourceTracker& tracker)
{
    while (true) {

        // Make a list of the passes we're selecting from
        std::vector<Handle<RenderPass>> nodes;

        const auto currentLength = iter.begin()->first;

        for (auto [length, pass] : iter) {
            if (length == currentLength) {
                nodes.push_back(pass);
            } else {
                break;
            }
        }

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
    }
}

void
RenderGraph::add_pass_to_plan(Handle<RenderPass> handle,
                              ExecutionPlan& plan,
                              PassGraph::Iterator& iter,
                              Resources::BranchingResourceTracker& tracker)
{
    iter.mark_finished(handle);

    const auto& pass = passes[handle];

    ExecutionPlan::ExecutePass execution;

    // If write: mustn't be dirty or being read
    // If read: mustn't be dirty
    // Either way state must be compatible
    // If queue is different we always need a transition

    // We can merge two barriers if and only if we're merging several reads, the
    // image layout is the same and on the same queue

    auto read_helper = [&](auto& reads,
                           auto& transfers,
                           auto& barriers,
                           auto& merges) {
        for (const auto& read : reads) {

            // Do not edit this reference
            const auto& state = tracker.state.get(read.id);

            // Do we need to transition the resource to this queue?
            if (state.queue != pass.queue) {
                // Add queue transition
                transfers.push_back(create_transition(read, state, tracker));

                tracker.state.set(read.id, read.access);
            }
            // Do we need to flush data and / or transition layout
            else if (!is_state_compatible(
                         read.id, read.access, state, tracker)) {
                barriers.push_back(create_barrier(read.id, read.access, state));

                tracker.state.set(read.id, read.access);
            }
            // Do we need to merge our read flags with a prior passes barrier?
            else if (is_merge_needed(read.access, state)) {
                merges.emplace_back(
                    read.id, read.access.access, read.access.stages);

                auto copy = state;
                copy.stages |= read.access.stages;
                copy.access |= read.access.access;

                tracker.state.set(read.id, copy);
            }

            tracker.dirty.set(read.id, false);
            tracker.lastPassToAccessResource.set(read.id, pass.id);
        }
    };

    read_helper(pass.bufferReads,
                execution.bufferTransfers,
                execution.bufferBarriers,
                execution.bufferMerges);
    read_helper(pass.imageReads,
                execution.imageTransfers,
                execution.imageBarriers,
                execution.imageMerges);

    for (const auto& write : pass.bufferWrites) {
        auto state = tracker.state.get(write.id);

        // A write actually always needs a barrier prior to starting, with the
        // only exception if it is the first pass to write
        // This is marked the UINT32_MAX handle
        // As such we only care about this and queue transitions

        if (tracker.lastPassToAccessResource.get(write.id).index ==
            UINT32_MAX) {
            // If unaccessed, we just need to set state
        }
        // Insert transfer else just a barrier
        else if (pass.queue != state.queue) {
            execution.bufferTransfers.push_back(
                create_transition(write.id, write.access, state, tracker));
        } else {
            execution.bufferBarriers.push_back(
                create_barrier(write.id, write.access, state));
        }

        tracker.state.set(write.id, write.access);
        tracker.dirty.set(write.id, true);
        tracker.lastPassToAccessResource.set(write.id, pass.id);
    }

    plan.push(execution);
}

bool
RenderGraph::is_state_compatible(Handle<AllocatedBuffer> handle,
                                 const BufferAccess& access,
                                 const BufferAccess& state,
                                 Resources::BranchingResourceTracker& tracker)
{
    return !tracker.dirty.get(handle);
}

bool
RenderGraph::is_state_compatible(Handle<AllocatedImage> handle,
                                 const ImageAccess& access,
                                 const ImageAccess& state,
                                 Resources::BranchingResourceTracker& tracker)
{
    if (tracker.dirty.get(handle)) {
        return false;
    }
    if (access.layout != state.layout) {
        return false;
    }

    return true;
}

bool
RenderGraph::is_merge_needed(const BufferAccess& access,
                             const BufferAccess& state)
{
    // check if our flags are a subset of the existing state
    if ((access.access & state.access) == access.access) {
        return false;
    }
    if ((access.stages & state.stages) == access.stages) {
        return false;
    }
    return true;
}

bool
RenderGraph::is_merge_needed(const ImageAccess& access,
                             const ImageAccess& state)
{
    // check if our flags are a subset of the existing state
    if ((access.access & state.access) == access.access) {
        return true;
    }
    if ((access.stages & state.stages) == access.stages) {
        return true;
    }
    return false;
}

BufferTransfer
RenderGraph::create_transition(const Handle<AllocatedBuffer> handle,
                               const BufferAccess& access,
                               const BufferAccess& state,
                               Resources::BranchingResourceTracker& tracker)
{

    return { .semaphore = tracker.lastPassToAccessResource.get(handle),
             .barrier = { .srcStageMask = state.stages,
                          .srcAccessMask = state.access,
                          .dstStageMask = access.stages,
                          .dstAccessMask = access.access,
                          .srcQueueFamilyIndex = state.queue,
                          .dstQueueFamilyIndex = access.queue,
                          .buffer = handle } };
}

ImageTransfer
RenderGraph::create_transition(const Handle<AllocatedImage> handle,
                               const ImageAccess& access,
                               const ImageAccess& state,
                               Resources::BranchingResourceTracker& tracker)
{
    return { .semaphore = tracker.lastPassToAccessResource.get(handle),
             .barrier = { .srcStageMask = state.stages,
                          .srcAccessMask = state.access,
                          .dstStageMask = access.stages,
                          .dstAccessMask = access.access,
                          .oldLayout = state.layout,
                          .newLayout = access.layout,
                          .srcQueueFamilyIndex = state.queue,
                          .dstQueueFamilyIndex = access.queue,
                          .image = handle } };
}

BufferBarrier
RenderGraph::create_barrier(const Handle<AllocatedBuffer> handle,
                            const BufferAccess& access,
                            const BufferAccess& state)
{
    return { .srcStageMask = state.stages,
             .srcAccessMask = state.access,
             .dstStageMask = access.stages,
             .dstAccessMask = access.access,
             .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
             .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
             .buffer = handle };
}

ImageBarrier
RenderGraph::create_barrier(const Handle<AllocatedImage> handle,
                            const ImageAccess& access,
                            const ImageAccess& state)
{
    return { .srcStageMask = state.stages,
             .srcAccessMask = state.access,
             .dstStageMask = access.stages,
             .dstAccessMask = access.access,
             .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
             .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
             .image = handle };
}
