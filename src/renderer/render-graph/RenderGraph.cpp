#include "renderer/render-graph/RenderGraph.h"

#include "renderer/Queues.h"
#include "renderer/render-graph/ExecutionPlan.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/resource-management/ResourceTracker.h"

#include <ranges>

using namespace Cel::Renderer;
using namespace Cel::Renderer::RenderGraph;

Cel::Common::RelativeScheduler<Cel::Handle<RenderPass>,
                               Cel::Common::Graph<Cel::Handle<RenderPass>>>
Graph::add_pass(const RenderPass& pass)
{
    passes.emplace(pass.id, pass);
    graph.add_edge(Passes::setupPass, pass.id);
    return add_system(pass.id);
}

Cel::Common::RelativeScheduler<Cel::Handle<RenderPass>,
                               Cel::Common::Graph<Cel::Handle<RenderPass>>>
Graph::add_setup_pass(const RenderPass& pass)
{
    passes.emplace(pass.id, pass);
    setupPasses.insert(pass.id);

    graph.add_edge(pass.id, Passes::setupPass);
    return add_system(pass.id);
}

void
Graph::set_present_pass(const RenderPass& pass)
{
    passes.emplace(pass.id, pass);
    presentPass = pass.id;
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

    // The setup pass is a dummy pass, but still needs initialisation
    passes.emplace(Passes::setupPass, Passes::setupPass);

    // Add the specialisations for each queue for resource transfers
    add_setup_chain(RenderPass{ .id = Passes::basePassGraphics,
                                .queue = Queues::graphics.family },
                    RenderPass{ .id = Passes::basePassCompute,
                                .queue = Queues::compute.family },
                    RenderPass{ .id = Passes::basePassTransfer,
                                .queue = Queues::transfer.family });

    for (const auto& [pass, node] : graph.reverseAdjacencyList) {

        // If this node is a root node, run after base pass
        if (node.empty() && pass != Passes::basePassGraphics &&
            pass != Passes::basePassCompute &&
            pass != Passes::basePassTransfer) {
            graph.add_edge(Passes::basePassTransfer, pass);
        }
    }

    // Set last pass in graph
    graph.add_node(presentPass);

    for (const auto& [pass, node] : graph.adjacencyList) {

        // If this node is a final node, add an edge to the last pass
        if (node.empty() && pass != presentPass) {
            graph.add_edge(pass, presentPass);
        }
    }

    compile_passes(manager);

    auto tracker = manager.branch_tracker();

    ExecutionPlan plan{};

    auto iter = graph.iter();

    search_branch(iter, tracker, plan, manager);

    for (const auto& pass : finalPlan) {
        fmt::println("{} {}",
                     Passes::HandleAllocator::get_name(pass.pass),
                     pass.pass.index);
    }
    fmt::println("end");
}

void
Graph::execute(PassServer& passServer,
               Swapchain& swapchain,
               VulkanResourceManager& manager)
{
    ExecutionPlan::execute(
        finalPlan, passes, presentPass, passServer, swapchain, manager);
}

void
Graph::reset()
{
    passes.clear();
    bufferHandleToMapped.clear();
    imageHandleToMapped.clear();
    perFrameBuffers.clear();
    perFrameImages.clear();
    finalPlan.clear();
    bestCost = UINT32_MAX;
}

void
Graph::compile_passes(VulkanResourceManager& manager)
{
    // We mark the state as coming from a base pass, the default state shows the
    // resource is untouched
    auto create_helper = [&](auto& creates, auto& mapping, auto& perFrames) {
        for (auto& create : creates) {
            auto handle = manager.get_handle_from_requirements(
                create.requirements,
                Passes::HandleAllocator::get_name(create.id));
            mapping[create.id] = handle;

            if (create.perFrame) {
                perFrames.emplace(handle);
            }

            create.id = handle;
        }
    };

    // We firstly need to create the actual mapping from handle to mapped handle
    for (auto& pass : passes | std::views::values) {
        create_helper(
            pass.bufferCreates, bufferHandleToMapped, perFrameBuffers);
        create_helper(pass.imageCreates, imageHandleToMapped, perFrameImages);
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
                     ExecutionPlan& plan,
                     VulkanResourceManager& manager)
{

    // Make a list of the passes we're selecting from
    std::vector<Handle<RenderPass>> nodes;
    while (true) {
        nodes.clear();

        // If we've finished the plan, ...
        if (iter.begin() == iter.end()) {
            if (plan.cost() < bestCost) {
                bestCost = plan.cost();

                finalPlan = plan.compile();
                manager.update(tracker);
            }
            return;
        }

        const auto greatestCriticalPath = iter.begin()->first;

        for (auto [length, pass] : iter) {
            // Only select from nodes with a matching path length
            if (length == greatestCriticalPath) {
                nodes.push_back(pass);
            } else {
                break;
            }
        }

        // The only case where setup should be available is when all setup
        // passes have run, in which case it will be the only available node

        if (nodes[0] == Passes::setupPass) {
            iter.mark_finished(Passes::setupPass);
            continue;
        }

        // No need to branch if it's linear
        if (nodes.size() == 1) {
            add_pass_to_plan(nodes[0], plan, iter, tracker);
            continue;
        }

        // Create a new branch for each pass
        for (const auto& handle : nodes) {
            auto branchIter = iter.branch_off();

            tracker.add_checkpoint(handle);

            auto branchPlan = plan.branch_off();

            add_pass_to_plan(handle, branchPlan, branchIter, tracker);
            search_branch(branchIter, tracker, branchPlan, manager);

            tracker.rewind();
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

    const auto& pass = passes.at(handle);

    ExecutionPlan::ExecutePass execution{ .pass = handle };

    // If write: mustn't be dirty or being read
    // If read: mustn't be dirty
    // Either way state must be compatible
    // If queue is different we always need a transition

    // We can merge two barriers if and only if we're merging several reads, the
    // image layout is the same and on the same queue

    auto create_helper = [&](auto& creates) {
        for (const auto& create : creates) {
            tracker.manifest_resource(create.id);
        }
    };

    create_helper(pass.bufferCreates);
    create_helper(pass.imageCreates);

    auto read_helper = [&](auto& reads,
                           auto& transfers,
                           auto& barriers,
                           auto& merges) {
        for (const auto& read : reads) {

            const auto& state = tracker.get_state(read.id);

            // Do we need to transition the resource to this queue?
            if (read.access.queue != state.queue &&
                state.queue != VK_QUEUE_FAMILY_IGNORED) {
                // Add queue transition
                transfers.push_back(
                    create_transition(read.id, read.access, state, tracker));

                tracker.set_state(read.id, read.access);
            }
            // Do we need to flush data and / or transition layout
            else if (!is_state_compatible(
                         read.id, read.access, state, tracker)) {
                barriers.push_back(create_barrier(read.id, read.access, state));

                tracker.set_state(read.id, read.access);
            }
            // Do we need to merge our read flags with a prior passes barrier?
            else if (is_merge_needed(read.access, state)) {
                merges.emplace_back(
                    read.id, read.access.access, read.access.stages);

                auto copy = state;
                copy.stages |= read.access.stages;
                copy.access |= read.access.access;

                tracker.set_state(read.id, copy);
            }

            tracker.mark_clean(read.id);
            tracker.set_last_access(read.id, pass.id);
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

            auto state = tracker.get_state(write.id);

            // We always need a barrier before a write. The only exception is
            // when the resource is untouched i.e. QUEUE_FAMILY_IGNORED. Even
            // then images still need their layout transitioned
            if (!is_write_barrier_needed(write.id, tracker)) {
                // If unaccessed, we just need to set state
            }
            // Insert transfer else just a barrier
            else if (write.access.queue != state.queue &&
                     state.queue != VK_QUEUE_FAMILY_IGNORED) {
                transfers.push_back(
                    create_transition(write.id, write.access, state, tracker));
            } else {
                barriers.push_back(
                    create_barrier(write.id, write.access, state));
            }

            tracker.set_state(write.id, write.access);
            tracker.mark_dirty(write.id);
            tracker.set_last_access(write.id, pass.id);
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
    return !tracker.is_dirty(handle);
}

bool
Graph::is_state_compatible(const Handle<AllocatedImage> handle,
                           const ImageAccess& access,
                           const ImageAccess& state,
                           BranchingResourceTracker& tracker)
{
    if (tracker.is_dirty(handle)) {
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
    return tracker.get_state(handle).queue != VK_QUEUE_FAMILY_IGNORED;
}

bool
Graph::is_write_barrier_needed(Handle<AllocatedImage> handle,
                               BranchingResourceTracker& tracker)
{
    // If there's no existing state, then the layout would be incorrect
    return true;
}

BufferTransfer
Graph::create_transition(const Handle<AllocatedBuffer> handle,
                         const BufferAccess& access,
                         const BufferAccess& state,
                         BranchingResourceTracker& tracker)
{
    auto signal = tracker.get_last_access(handle);

    if (signal == Passes::basePass) {
        if (state.queue == Queues::graphics.family) {
            signal = Passes::basePassGraphics;
        } else if (state.queue == Queues::compute.family) {
            signal = Passes::basePassCompute;
        } else if (state.queue == Queues::transfer.family) {
            signal = Passes::basePassTransfer;
        }
    }

    return { .signalPass = signal,
             .barrier = { .srcStageMask = state.stages,
                          .srcAccessMask = state.access,
                          .dstStageMask = access.stages,
                          .dstAccessMask = access.access,
                          .srcQueueFamilyIndex = state.queue,
                          .dstQueueFamilyIndex = access.queue,
                          .handle = handle } };
}

ImageTransfer
Graph::create_transition(const Handle<AllocatedImage> handle,
                         const ImageAccess& access,
                         const ImageAccess& state,
                         BranchingResourceTracker& tracker)
{
    auto signal = tracker.get_last_access(handle);

    if (signal == Passes::basePass) {
        if (state.queue == Queues::graphics.family) {
            signal = Passes::basePassGraphics;
        } else if (state.queue == Queues::compute.family) {
            signal = Passes::basePassCompute;
        } else if (state.queue == Queues::transfer.family) {
            signal = Passes::basePassTransfer;
        }
    }

    return { .signalPass = signal,
             .barrier = { .srcStageMask = state.stages,
                          .srcAccessMask = state.access,
                          .dstStageMask = access.stages,
                          .dstAccessMask = access.access,
                          .oldLayout = state.layout,
                          .newLayout = access.layout,
                          .srcQueueFamilyIndex = state.queue,
                          .dstQueueFamilyIndex = access.queue,
                          .handle = handle } };
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
             .handle = handle };
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
             .oldLayout = state.layout,
             .newLayout = access.layout,
             .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
             .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
             .handle = handle };
}
