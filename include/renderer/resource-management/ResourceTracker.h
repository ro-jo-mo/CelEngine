#pragma once

#include "common/Handle.h"
#include "renderer/VulkanTypes.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/RenderGraphTypes.h"

#include <unordered_set>

namespace Cel::Renderer {

// Tracks the current state of resources at a point in the graph
class ResourceTracker
{
  public:
    BufferAccess get_state(Handle<AllocatedBuffer> buffer);

    ImageAccess get_state(Handle<AllocatedImage> image);

    void set_state(Handle<AllocatedBuffer> handle, const BufferAccess& access);

    void set_state(Handle<AllocatedImage> handle, const ImageAccess& access);

  protected:
    // Track the current state of images and buffers
    std::unordered_map<Handle<AllocatedBuffer>, BufferAccess> buffers;
    std::unordered_map<Handle<AllocatedImage>, ImageAccess> images;

    friend class BranchingResourceTracker;
};

// A branching version of the resource tracker that allows us to keep track of
// resources throughout different branches
// It is key to note if we want thread safe use, when presented with a branch
// splitting two ways, we must branch off twice, so the original tracker is
// untouched
class BranchingResourceTracker
{
  public:
    explicit BranchingResourceTracker(const ResourceTracker& tracker);

    // Create a new resource tracker representing a separate branch
    BranchingResourceTracker branch_off();

    // Stores a branch of state data
    // To avoid altering the state of previous branches in the tree we store
    // pointers to the original, but only edit this copy
    template<typename BufValue, typename ImgValue>
    struct Branch
    {
        explicit Branch(Branch* original);

        // Initial values are used in the case when the value does not exist yet
        explicit Branch(BufValue initialBuf, ImgValue initialImg);

        // Special case where we inherit from our resource manager. Makes the
        // assumption that all resources are initialised
        explicit Branch(
            const std::unordered_map<Handle<AllocatedBuffer>, BufValue>&
                buffers,
            const std::unordered_map<Handle<AllocatedImage>, ImgValue>& images);

        void set(Handle<AllocatedBuffer> handle, BufValue value);
        void set(Handle<AllocatedImage> handle, ImgValue value);

        [[nodiscard]] const BufValue& get(Handle<AllocatedBuffer> handle);
        [[nodiscard]] const ImgValue& get(Handle<AllocatedImage> handle);

        Branch* original = nullptr;

        std::unordered_map<Handle<AllocatedBuffer>, BufValue> buffers;
        std::unordered_map<Handle<AllocatedImage>, ImgValue> images;

        BufValue initialBuf;
        ImgValue initialImg;
    };

    // Stores whether a resource is ready to be reused
    Branch<bool, bool> reusable{ false, false };

    // Stores whether a resource has been written to, and needs flushing
    Branch<bool, bool> dirty{ false, false };

    // The current state of this resource
    Branch<BufferAccess, ImageAccess> state;

    // The last pass to read or write this resource
    // If a resource is not here, we assume it's state is from a prior frame
    Branch<Handle<RenderGraph::RenderPass>, Handle<RenderGraph::RenderPass>>
        lastPassToAccessResource{ Passes::basePass, Passes::basePass };

  private:
    BranchingResourceTracker(const BranchingResourceTracker& tracker);
};

template<typename BufValue, typename ImgValue>
BranchingResourceTracker::Branch<BufValue, ImgValue>::Branch(Branch* original)
    : original(original)
{
}

template<typename BufValue, typename ImgValue>
BranchingResourceTracker::Branch<BufValue, ImgValue>::Branch(
    BufValue initialBuf,
    ImgValue initialImg)
    : original(nullptr)
    , initialBuf(initialBuf)
    , initialImg(initialImg)
{
}

template<typename BufValue, typename ImgValue>
BranchingResourceTracker::Branch<BufValue, ImgValue>::Branch(
    const std::unordered_map<Handle<AllocatedBuffer>, BufValue>& buffers,
    const std::unordered_map<Handle<AllocatedImage>, ImgValue>& images)
    : original(nullptr)
    , buffers(buffers)
    , images(images)
{
}

template<typename BufValue, typename ImgValue>
void
BranchingResourceTracker::Branch<BufValue, ImgValue>::set(
    Handle<AllocatedBuffer> handle,
    BufValue value)
{
    buffers[handle] = value;
}

template<typename BufValue, typename ImgValue>
void
BranchingResourceTracker::Branch<BufValue, ImgValue>::set(
    Handle<AllocatedImage> handle,
    ImgValue value)
{
    images[handle] = value;
}

template<typename BufValue, typename ImgValue>
const BufValue&
BranchingResourceTracker::Branch<BufValue, ImgValue>::get(
    Handle<AllocatedBuffer> handle)
{
    if (buffers.contains(handle)) {
        return buffers[handle];
    }
    if (original != nullptr) {
        return original->get(handle);
    }

    return initialBuf;
}

template<typename BufValue, typename ImgValue>
const ImgValue&
BranchingResourceTracker::Branch<BufValue, ImgValue>::get(
    Handle<AllocatedImage> handle)
{
    if (images.contains(handle)) {
        return images[handle];
    }
    if (original != nullptr) {
        return original->get(handle);
    }

    return initialImg;
}

}