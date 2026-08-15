#pragma once

#include "ExecutionPlan.h"
#include "PassGraph.h"
#include "RenderPass.h"
#include "common/Handle.h"
#include "renderer/VulkanTypes.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <string>
#include <unordered_map>

namespace Cel::Renderer {

class RenderGraph
{
  public:
    void add_pass(const RenderPass& pass);

    /**
     * Creates a plan for executing the graph
     * The primary purpose is to decide on an optimal ordering of passes
     * Additionally when to insert synchronisation primitives
     */
    void compile();

    // Finally execute the render passes
    void execute();

  private:
    void compile_passes(PassGraph::Iterator& iter);

    void search_branch(PassGraph::Iterator iter,
                       Resources::BranchingResourceTracker& tracker);

    void add_pass(Handle<RenderPass> handle,
                  ExecutionPlan& plan,
                  PassGraph::Iterator& iter,
                  Resources::BranchingResourceTracker& tracker);

    bool is_barrier_needed(Handle<AllocatedBuffer> handle,
                           const BufferAccess& access,
                           Resources::BranchingResourceTracker& tracker);

    bool is_barrier_needed(Handle<AllocatedImage> handle,
                           const ImageAccess& access,
                           Resources::BranchingResourceTracker& tracker);

    bool is_state_compatible(BufferAccess wanted, BufferAccess current);

    bool is_state_compatible(ImageAccess wanted, ImageAccess current);

    Handle<AllocatedBuffer> get_buffer_handle_from_name(std::string name);
    Handle<AllocatedImage> get_image_handle_from_name(std::string name);

    // Throughout the rendergraph resources will be written to, and adopt a new
    // name post write, which in turn implements the ordering of passes and
    // barriers
    // As such multiple names map to the same resource

    // We want to firstly store a mapping of all names to a unique handle
    std::unordered_map<std::string, Handle<AllocatedBuffer>> bufferNameToHandle;
    std::unordered_map<std::string, Handle<AllocatedImage>> imageNameToHandle;

    // Simply a list of all render passes
    std::vector<RenderPass> passes;
    std::vector<RenderPassCompiled> compiledPasses;

    ExecutionPlan plan;

    VkDevice device;
    VmaAllocator allocator;
    Resources::VulkanResourceManager& resourceManager;

    friend class PassBuilder;
};

}