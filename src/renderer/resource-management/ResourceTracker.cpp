#include "renderer/resource-management/ResourceTracker.h"

#include <ranges>

Cel::Renderer::BufferAccess
Cel::Renderer::ResourceTracker::get_state(Handle<AllocatedBuffer> buffer)
{
    return buffers[buffer];
}

Cel::Renderer::ImageAccess
Cel::Renderer::ResourceTracker::get_state(Handle<AllocatedImage> image)
{
    return images[image];
}

void
Cel::Renderer::ResourceTracker::set_state(Handle<AllocatedBuffer> handle,
                                          const BufferAccess& access)
{
    buffers[handle] = access;
}

void
Cel::Renderer::ResourceTracker::set_state(Handle<AllocatedImage> handle,
                                          const ImageAccess& access)
{
    images[handle] = access;
}

Cel::Renderer::BranchingResourceTracker
Cel::Renderer::BranchingResourceTracker::branch_off()
{
    return { *this };
}

Cel::Renderer::ResourceTracker
Cel::Renderer::BranchingResourceTracker::compile(ResourceTracker& original)
{
    auto copy = original;

    for (const auto& handle : original.buffers | std::views::keys) {
        copy.set_state(handle, state.get(handle));
    }
    for (const auto& handle : original.images | std::views::keys) {
        copy.set_state(handle, state.get(handle));
    }

    return copy;
}

Cel::Renderer::BranchingResourceTracker::BranchingResourceTracker(
    const ResourceTracker& tracker)
    : state(tracker.buffers, tracker.images)
{
}

Cel::Renderer::BranchingResourceTracker::BranchingResourceTracker(
    const BranchingResourceTracker& tracker) = default;
