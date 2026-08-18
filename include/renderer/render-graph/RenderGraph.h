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
    Common::RelativeScheduler<Handle<RenderPass>, Scheduler> add_pass(
        const RenderPass& pass);

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

    void search_branch(PassGraph::Iterator iter,
                       Resources::BranchingResourceTracker& tracker);

    void _add_pass(Handle<RenderPass> handle,
                   ExecutionPlan& plan,
                   PassGraph::Iterator& iter,
                   Resources::BranchingResourceTracker& tracker);

    bool is_barrier_needed(const BufferRead& access,
                           Resources::BranchingResourceTracker& tracker);
    bool is_barrier_needed(const ImageRead& access,
                           Resources::BranchingResourceTracker& tracker);
    bool is_barrier_needed(const BufferWrite& access,
                           Resources::BranchingResourceTracker& tracker);
    bool is_barrier_needed(const ImageWrite& access,
                           Resources::BranchingResourceTracker& tracker);

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
