#include "renderer/render-graph/RenderGraph.h"

#include "renderer/render-graph/ExecutionPlan.h"
#include "renderer/resource-management/ResourceTracker.h"

#include <map>
#include <ranges>

using namespace Cel::Renderer;

void
RenderGraph::add_pass(const RenderPass& pass)
{
    passes.push_back(pass);
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

    PassGraph passGraph{ passes };

    auto iter = passGraph.iter();

    compile_passes(iter);

    iter.reset();

    Resources::BranchingResourceTracker tracker =
        resourceManager.branch_tracker();

    search_branch(iter, tracker);
}

void
RenderGraph::compile_passes(PassGraph::Iterator& iter)
{
    compiledPasses.resize(passes.size());

    auto create_helper = [this](auto& addTo, const auto& creates, auto& map) {
        for (const auto& create : creates) {
            auto handle = resourceManager.get_handle_from_requirements(
                create.requirements);

            map[create.name] = handle;

            addTo.emplace_back(handle, create.requirements);
        }
    };

    auto write_helper = [](auto& addTo, const auto& writes, auto& map) {
        for (const auto& write : writes) {
            auto handle = map[write.inName];

            map[write.outName] = handle;
            // Silly duplicate here :(
            addTo.emplace_back(handle, handle, write.access);
        }
    };

    auto read_helper = [](auto& addTo, const auto& reads, auto& map) {
        for (const auto& read : reads) {
            addTo.emplace_back(map[read.name], read.access);
        }
    };

    for (const auto& nodes = iter.get_available_nodes(); !nodes.empty();) {

        for (const auto handle : nodes | std::views::values) {
            iter.mark_finished(handle);

            auto& pass = passes[handle.index];

            RenderPassCompiled comp;

            comp.bufferReads.resize(pass.bufferReads.size());
            comp.bufferWrites.resize(pass.bufferWrites.size());
            comp.imageWrites.resize(pass.imageWrites.size());
            comp.imageReads.resize(pass.imageReads.size());
            comp.newBuffers.resize(pass.newBuffers.size());
            comp.newImages.resize(pass.newImages.size());
            comp.execute = pass.execute;

            create_helper(comp.newBuffers, pass.newBuffers, bufferNameToHandle);
            create_helper(comp.newImages, pass.newImages, imageNameToHandle);

            write_helper(
                comp.bufferWrites, pass.bufferWrites, bufferNameToHandle);
            write_helper(comp.imageWrites, pass.imageWrites, imageNameToHandle);

            read_helper(comp.bufferReads, pass.bufferReads, bufferNameToHandle);
            read_helper(comp.imageReads, pass.imageReads, imageNameToHandle);

            compiledPasses[handle.index] = comp;
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
    // is it dirty?
}

bool
RenderGraph::is_barrier_needed(const Handle<AllocatedBuffer> handle,
                               const BufferAccess& access,
                               Resources::BranchingResourceTracker& tracker)
{
    if (tracker.is_dirty(handle)) {
        return true;
    }

    auto state = tracker.get_buffer_state(handle);

    if (state.queue != access.queue) {
        return true;
    }

    return false;
}

bool
RenderGraph::is_barrier_needed(const Handle<AllocatedImage> handle,
                               const ImageAccess& access,
                               Resources::BranchingResourceTracker& tracker)
{
    if (tracker.is_dirty(handle)) {
        return true;
    }

    auto state = tracker.get_image_state(handle);

    if (state.queue != access.queue) {
        return true;
    }
    if (state.layout != access.layout) {
        return true;
    }

    return false;
}
