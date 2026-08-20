#pragma once

#include "ExecutionPlan.h"
#include "PassGraph.h"
#include "RenderPass.h"
#include "common/Handle.h"
#include "common/Scheduler.h"
#include "renderer/VulkanTypes.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <string>
#include <unordered_map>

namespace Cel::Renderer {
class RenderGraph : Common::Scheduler<Handle<RenderPass>>
{
  public:
    Common::RelativeScheduler<Handle<RenderPass>,
                              Common::Graph<Handle<RenderPass>>>
    add_pass(const RenderPass& pass);

    template<typename... Passes>
    Common::RelativeScheduler<Handle<RenderPass>, Scheduler> add_chain(
        Passes... pass);

    /**
     * Creates a plan for executing the graph
     * The primary purpose is to decide on an optimal ordering of passes
     * Additionally when to insert synchronisation primitives
     */
    void compile();

    // Finally execute the render passes
    void execute();

  private:
    // Resolve the buffer and image handles to handles to actual vulkan
    // resources
    void compile_passes();

    void search_branch(Common::Graph<Handle<RenderPass>>::Iterator iter,
                       Resources::BranchingResourceTracker& tracker);

    void add_pass_to_plan(Handle<RenderPass> handle,
                          ExecutionPlan& plan,
                          PassGraph::Iterator& iter,
                          Resources::BranchingResourceTracker& tracker);

    static bool is_state_compatible(
        Handle<AllocatedBuffer> handle,
        const BufferAccess& access,
        const BufferAccess& state,
        Resources::BranchingResourceTracker& tracker);

    static bool is_state_compatible(
        Handle<AllocatedImage> handle,
        const ImageAccess& access,
        const ImageAccess& state,
        Resources::BranchingResourceTracker& tracker);

    static bool is_merge_needed(const BufferAccess& access,
                                const BufferAccess& state);
    static bool is_merge_needed(const ImageAccess& access,
                                const ImageAccess& state);

    static BufferTransfer create_transition(
        Handle<AllocatedBuffer> handle,
        const BufferAccess& access,
        const BufferAccess& state,
        Resources::BranchingResourceTracker& tracker);

    static ImageTransfer create_transition(
        Handle<AllocatedImage> handle,
        const ImageAccess& access,
        const ImageAccess& state,
        Resources::BranchingResourceTracker& tracker);

    static BufferBarrier create_barrier(Handle<AllocatedBuffer> handle,
                                        const BufferAccess& access,
                                        const BufferAccess& state);
    static ImageBarrier create_barrier(Handle<AllocatedImage> handle,
                                       const ImageAccess& access,
                                       const ImageAccess& state);

    // A mapping of pass id's to the actual pass data
    std::unordered_map<Handle<RenderPass>, RenderPass> passes;

    // Converts the resource handles used by render passes to actual handles to
    // vulkan resources
    std::unordered_map<Handle<AllocatedBuffer>, Handle<AllocatedBuffer>>
        bufferHandleToMapped;
    std::unordered_map<Handle<AllocatedImage>, Handle<AllocatedImage>>
        imageHandleToMapped;

    VkDevice device;
    VmaAllocator allocator;
    Resources::VulkanResourceManager& resourceManager;

    friend class PassBuilder;
};

template<typename... Passes>
Common::RelativeScheduler<Handle<RenderPass>,
                          Common::Scheduler<Handle<RenderPass>>>
RenderGraph::add_chain(Passes... pass)
{
    (void(passes.insert({ pass.id, pass })), ...);

    return add_chain(pass.id...);
}
}
