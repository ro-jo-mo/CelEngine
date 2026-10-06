#pragma once

#include "VulkanResourceManager.h"
#include "common/Handle.h"
#include "renderer/VulkanTypes.h"

#include <optional>
#include <variant>

namespace Cel::Renderer {

// As we traverse render graph

// create: this resource exists, with this state (either fresh or aliased or
// already existing)
// At read, this is clean
// At write, this is dirty
// At r/w, the state is now:

// Throughout we need to be able to backtrack as we traverse
// different branches

class BranchingResourceTracker : VulkanResourceManager
{
  public:
    // As this does zero allocations, init as null
    explicit BranchingResourceTracker(const VulkanResourceManager& manager);

    // Decide whether this resource will be aliased or created new
    void manifest_resource(Handle<AllocatedBuffer> handle);
    void manifest_resource(Handle<AllocatedImage> handle);

    [[nodiscard]] bool is_dirty(Handle<AllocatedBuffer> handle) const;
    [[nodiscard]] bool is_dirty(Handle<AllocatedImage> handle) const;

    void mark_dirty(Handle<AllocatedBuffer> handle);
    void mark_dirty(Handle<AllocatedImage> handle);

    void mark_clean(Handle<AllocatedBuffer> handle);
    void mark_clean(Handle<AllocatedImage> handle);

    [[nodiscard]] const BufferAccess& get_state(
        Handle<AllocatedBuffer> handle) const;
    [[nodiscard]] const ImageAccess& get_state(
        Handle<AllocatedImage> handle) const;

    void set_state(Handle<AllocatedBuffer> handle, const BufferAccess& state);
    void set_state(Handle<AllocatedImage> handle, const ImageAccess& state);

    [[nodiscard]] Handle<RenderGraph::RenderPass> get_last_access(
        Handle<AllocatedBuffer> handle) const;
    [[nodiscard]] Handle<RenderGraph::RenderPass> get_last_access(
        Handle<AllocatedImage> handle) const;

    void set_last_access(Handle<AllocatedBuffer> handle,
                         Handle<RenderGraph::RenderPass> pass);
    void set_last_access(Handle<AllocatedImage> handle,
                         Handle<RenderGraph::RenderPass> pass);

    void add_checkpoint(Handle<RenderGraph::RenderPass> handle);

    void rewind();

  private:
    std::unordered_map<Handle<AllocatedBuffer>, Handle<RenderGraph::RenderPass>>
        lastPassToAccessBuffer;
    std::unordered_map<Handle<AllocatedImage>, Handle<RenderGraph::RenderPass>>
        lastPassToAccessImage;

    struct ManifestBuffer
    {
        Handle<AllocatedBuffer> handle;
        std::optional<
            std::tuple<AllocatedBuffer, BufferRequirements, BufferAccess>>
            aliased;
    };
    struct ManifestImage
    {
        Handle<AllocatedImage> handle;
        std::optional<
            std::tuple<AllocatedImage, ImageRequirements, ImageAccess>>
            aliased;
    };
    struct MarkBuffer
    {
        Handle<AllocatedBuffer> handle;
        bool marking;
    };
    struct MarkImage
    {
        Handle<AllocatedImage> handle;
        bool marking;
    };
    struct SetBufferState
    {
        Handle<AllocatedBuffer> handle;
        BufferAccess state;
    };
    struct SetImageState
    {
        Handle<AllocatedImage> handle;
        ImageAccess state;
    };
    struct LastBufferAccess
    {
        Handle<AllocatedBuffer> handle;
        Handle<RenderGraph::RenderPass> pass;
    };
    struct LastImageAccess
    {
        Handle<AllocatedImage> handle;
        Handle<RenderGraph::RenderPass> pass;
    };

    struct Checkpoint
    {
        Handle<RenderGraph::RenderPass> handle;
    };

    std::vector<std::variant<ManifestBuffer,
                             ManifestImage,
                             MarkBuffer,
                             MarkImage,
                             SetBufferState,
                             SetImageState,
                             LastBufferAccess,
                             LastImageAccess,
                             Checkpoint>>
        changeLog;

    friend class VulkanResourceManager;
};

template<class... Ts>
struct Overload : Ts...
{
    using Ts::operator()...;
};

// We must rework the tracker used by the rendergraph
// To allow for aliasing, we must have the full resource manager as part of the
// exploration strategy
// In the current strategy, we can't get the actual state of an object without
// materialising it, making it impossible to alias.

// Additional notes. If exploration is single threaded, ideally we only have one
// copy of the manager. It is mutated as we explore the graph, and changes are
// undone as we backtrack In a multi threaded strategy, we'd make copies upto
// the nth branch, each branch being a new thread. The individual threads would
// then retain the same mutating strategy

// I think I'll rename VulkanResourceManager to ResourceTracker.
// Resource track can be removed, it's just a simple map

// In the add_pass part of the render graph, we'll manifest the resources into
// allocations at the "create" stage We can't actually allocate them however, as
// that would be pointless.

// Eventually I'll add logic in the render graph to reuse resources inside the
// graph, i.e. marking resources "free" at the last point they're used This
// version of free is not the same as ResourceTracker.free

// We should store "dirtiness" in the main tracker

}