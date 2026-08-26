#pragma once

#include "ExecutionPlan.h"
#include "RenderGraphTypes.h"
#include "common/Handle.h"
#include "common/Scheduler.h"
#include "renderer/VulkanTypes.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <unordered_map>

namespace Cel::Renderer::RenderGraph {

// Overall flow:
// Add passes to graph each frame
// Compile graph
// Post compiling each pass can graph->get_cmd_buffer(pass) and record their
// commands get_resource to access buffers and images
// Execute finally submits the buffers
class Graph : Common::Scheduler<Handle<RenderPass>>
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

    void reset(uint32_t frame);

  private:
    // Resolve the buffer and image handles to handles to actual vulkan
    // resources
    void compile_passes(BranchingResourceTracker& tracker);

    void search_branch(Common::Graph<Handle<RenderPass>>::Iterator& iter,
                       BranchingResourceTracker& tracker,
                       ExecutionPlan& plan);

    void add_pass_to_plan(Handle<RenderPass> handle,
                          ExecutionPlan& plan,
                          Common::Graph<Handle<RenderPass>>::Iterator& iter,
                          BranchingResourceTracker& tracker);

    static bool is_state_compatible(Handle<AllocatedBuffer> handle,
                                    const BufferAccess& access,
                                    const BufferAccess& state,
                                    BranchingResourceTracker& tracker);

    static bool is_state_compatible(Handle<AllocatedImage> handle,
                                    const ImageAccess& access,
                                    const ImageAccess& state,
                                    BranchingResourceTracker& tracker);

    static bool is_merge_needed(const BufferAccess& access,
                                const BufferAccess& state);
    static bool is_merge_needed(const ImageAccess& access,
                                const ImageAccess& state);

    static BufferTransfer create_transition(Handle<AllocatedBuffer> handle,
                                            const BufferAccess& access,
                                            const BufferAccess& state,
                                            BranchingResourceTracker& tracker);

    static ImageTransfer create_transition(Handle<AllocatedImage> handle,
                                           const ImageAccess& access,
                                           const ImageAccess& state,
                                           BranchingResourceTracker& tracker);

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

    std::vector<ExecutionPlan::ExecutePass> finalPlan;
    uint32_t bestCost = UINT32_MAX;

    uint32_t currentFrame;

    VkDevice device;
    VmaAllocator allocator;
    VulkanResourceManager& resourceManager;

    friend class PassBuilder;
};

template<typename... Passes>
Common::RelativeScheduler<Handle<RenderPass>,
                          Common::Scheduler<Handle<RenderPass>>>
Graph::add_chain(Passes... pass)
{
    (void(passes.insert({ pass.id, pass })), ...);

    return add_chain(pass.id...);
}
}
