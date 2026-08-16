#pragma once

#include "common/Handle.h"
#include "renderer/VulkanTypes.h"
#include "renderer/render-graph/RenderPass.h"

#include <unordered_set>

namespace Cel::Renderer::Resources {

// Tracks the current state of resources at a point in the graph
class ResourceTracker
{
  public:
    virtual ~ResourceTracker() = default;

    virtual BufferAccess get_state(Handle<AllocatedBuffer> buffer);

    virtual ImageAccess get_state(Handle<AllocatedImage> image);

  protected:
    // Track the current state of images and buffers
    std::unordered_map<Handle<AllocatedBuffer>, BufferAccess> buffers;
    std::unordered_map<Handle<AllocatedImage>, ImageAccess> images;
};

// A branching version of the resource tracker that allows us to keep track of
// resources throughout different branches
// It is key to note if we want thread safe use, when presented with a branch
// splitting two ways, we must branch off twice, so the original tracker is
// untouched
class BranchingResourceTracker final : public ResourceTracker
{
  public:
    BufferAccess get_state(Handle<AllocatedBuffer> buffer) override;

    ImageAccess get_state(Handle<AllocatedImage> image) override;

    // Create a new resource tracker representing a separate branch
    BranchingResourceTracker branch_off();

    // Due to the branching nature of the tracker, a buffer may be marked
    // as reusable in the original, but then actually reused in this
    // branch. We cannot edit the original, so we must be able to mark a
    // resource as both reusable AND reused
    struct Branch
    {
        explicit Branch(Branch* original);

        void mark(Handle<AllocatedBuffer> handle);
        void mark(Handle<AllocatedImage> handle);

        void unmark(Handle<AllocatedBuffer> handle);
        void unmark(Handle<AllocatedImage> handle);

        [[nodiscard]] bool is_marked(Handle<AllocatedBuffer> handle) const;
        [[nodiscard]] bool is_marked(Handle<AllocatedImage> handle) const;

        std::unordered_set<Handle<AllocatedBuffer>> buffers;
        std::unordered_set<Handle<AllocatedImage>> images;

        std::unordered_set<Handle<AllocatedBuffer>> antiBuffers;
        std::unordered_set<Handle<AllocatedImage>> antiImages;

        Branch* original = nullptr;
    };

    // Mark a resource as finished with, and ready to be reused, unmark once
    // reused
    Branch reusable;

    // Mark a resource as being written to, unmark once a barrier is inserted
    Branch dirty;

    // Mark a resource as having been read, unmark once a write is added
    Branch read;

  private:
    explicit BranchingResourceTracker(ResourceTracker* tracker);
    explicit BranchingResourceTracker(BranchingResourceTracker* tracker);

    // Store reference to the original
    // When checking the current state of a resource, if state does not exist
    // yet in self, check original
    // This creates a pointer chain, ending with the non branching tracker
    ResourceTracker* original;
};

}