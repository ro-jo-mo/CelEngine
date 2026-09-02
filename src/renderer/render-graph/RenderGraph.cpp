#include "renderer/render-graph/RenderGraph.h"

#include "renderer/render-graph/ExecutionPlan.h"
#include "renderer/resource-management/ResourceTracker.h"

#include <map>
#include <ranges>

using namespace Cel::Renderer;
using namespace Cel::Renderer::RenderGraph;

Cel::Common::RelativeScheduler<Cel::Handle<RenderPass>,
                               Cel::Common::Graph<Cel::Handle<RenderPass>>>
Graph::add_pass(const RenderPass& pass)
{
    passes.insert({ pass.id, pass });
    return add_system(pass.id);
}

void
Graph::compile(VulkanResourceManager& manager)
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

    auto tracker = manager.branch_tracker();

    compile_passes(manager, tracker);

    ExecutionPlan plan{};

    // Insert the base plan
    // This is used to insert any barriers needed for across frame resources,
    // like the vertex buffer
    plan.push({ .pass = Passes::basePass });

    auto iter = graph.iter();

    search_branch(iter, tracker, plan);
}

void
Graph::compile_passes(VulkanResourceManager& manager,
                      BranchingResourceTracker& tracker)
{
    // We mark the state as coming from a null pass, meaning it has no existing
    // state and thus needs no synchronisation (apart from layout transition)
    auto create_helper = [&](auto& creates, auto& addTo) {
        for (auto& create : creates) {
            auto handle = manager.get_handle_from_requirements(
                create.requirements,
                Passes::HandleAllocator::get_name(create.id));
            addTo[create.id] = handle;
            tracker.lastPassToAccessResource.set(handle, Passes::nullPass);
        }
    };

    // We firstly need to create the actual mapping from handle to mapped handle
    for (auto& pass : passes | std::views::values) {
        create_helper(pass.newBuffers, bufferHandleToMapped);
        create_helper(pass.newImages, imageHandleToMapped);
    }

    auto to_mapped = [&](auto& iter, auto& map) {
        for (auto& it : iter) {
            if (map.contains(it.id)) {
                it.id = map.at(it.id);
            }
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
Graph::search_branch(Common::Graph<Handle<RenderPass>>::Iterator& iter,
                     BranchingResourceTracker& tracker,
                     ExecutionPlan& plan)
{

    // Make a list of the passes we're selecting from
    std::vector<Handle<RenderPass>> nodes;
    while (true) {
        nodes.clear();

        // If we've finished the plan, ...
        if (iter.begin() == iter.end()) {
            if (plan.cost() < bestCost) {
                finalPlan = plan.compile();
            }
            return;
        }

        const auto currentLength = iter.begin()->first;

        for (auto [length, pass] : iter) {
            if (length == currentLength) {
                nodes.push_back(pass);
            } else {
                break;
            }
        }

        // No need to branch if it's linear
        if (nodes.size() == 1) {
            add_pass_to_plan(nodes[0], plan, iter, tracker);
            continue;
        }

        // Create a new branch for each pass
        for (const auto& handle : nodes) {
            auto branchIter = iter.branch_off();
            auto branchTracker = tracker.branch_off();
            auto branchPlan = plan.branch_off();

            add_pass_to_plan(handle, branchPlan, branchIter, branchTracker);
            search_branch(branchIter, branchTracker, branchPlan);
        }

        // If we do branch, there's nothing more to do here, so just break
        break;
    }
}

void
Graph::add_pass_to_plan(const Handle<RenderPass> handle,
                        ExecutionPlan& plan,
                        Common::Graph<Handle<RenderPass>>::Iterator& iter,
                        BranchingResourceTracker& tracker)
{
    iter.mark_finished(handle);

    const auto& pass = passes[handle];

    ExecutionPlan::ExecutePass execution{ .pass = handle };

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

            const auto& state = tracker.state.get(read.id);

            // Do we need to transition the resource to this queue?
            if (read.access.queue != state.queue) {
                // Add queue transition
                transfers.push_back(
                    create_transition(read.id, read.access, state, tracker));

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

    auto write_helper = [&](auto& writes, auto& transfers, auto& barriers) {
        for (const auto& write : writes) {
            auto state = tracker.state.get(write.id);

            // We always need a barrier before a write. The only exception is
            // when this resource has no existing state, marked by nullPass.
            // Even then images still need their layout transitioned
            if (!is_write_barrier_needed(write.id, tracker)) {
                // If unaccessed, we just need to set state
            }
            // Insert transfer else just a barrier
            else if (write.access.queue != state.queue) {
                transfers.push_back(
                    create_transition(write.id, write.access, state, tracker));
            } else {
                barriers.push_back(
                    create_barrier(write.id, write.access, state));
            }

            tracker.state.set(write.id, write.access);
            tracker.dirty.set(write.id, true);
            tracker.lastPassToAccessResource.set(write.id, pass.id);
        }
    };

    write_helper(
        pass.bufferWrites, execution.bufferTransfers, execution.bufferBarriers);
    write_helper(
        pass.imageWrites, execution.imageTransfers, execution.imageBarriers);

    plan.push(execution);
}

bool
Graph::is_state_compatible(const Handle<AllocatedBuffer> handle,
                           const BufferAccess& access,
                           const BufferAccess& state,
                           BranchingResourceTracker& tracker)
{
    return !tracker.dirty.get(handle);
}

bool
Graph::is_state_compatible(const Handle<AllocatedImage> handle,
                           const ImageAccess& access,
                           const ImageAccess& state,
                           BranchingResourceTracker& tracker)
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
Graph::is_merge_needed(const BufferAccess& access, const BufferAccess& state)
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
Graph::is_merge_needed(const ImageAccess& access, const ImageAccess& state)
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

bool
Graph::is_write_barrier_needed(const Handle<AllocatedBuffer> handle,
                               BranchingResourceTracker& tracker)
{
    return tracker.lastPassToAccessResource.get(handle) == Passes::nullPass;
}

bool
Graph::is_write_barrier_needed(Handle<AllocatedImage> handle,
                               BranchingResourceTracker& tracker)
{
    return true;
}

BufferTransfer
Graph::create_transition(const Handle<AllocatedBuffer> handle,
                         const BufferAccess& access,
                         const BufferAccess& state,
                         BranchingResourceTracker& tracker)
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
Graph::create_transition(const Handle<AllocatedImage> handle,
                         const ImageAccess& access,
                         const ImageAccess& state,
                         BranchingResourceTracker& tracker)
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
Graph::create_barrier(const Handle<AllocatedBuffer> handle,
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
Graph::create_barrier(const Handle<AllocatedImage> handle,
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
