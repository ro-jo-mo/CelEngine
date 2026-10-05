#include "renderer/resource-management/ResourceTracker.h"

#include <ranges>

Cel::Renderer::BufferAccess
Cel::Renderer::ResourceTracker::get_state(
    const Handle<AllocatedBuffer> buffer) const
{
    return buffers.at(buffer);
}

Cel::Renderer::ImageAccess
Cel::Renderer::ResourceTracker::get_state(
    const Handle<AllocatedImage> image) const
{
    return images.at(image);
}

void
Cel::Renderer::ResourceTracker::set_state(const Handle<AllocatedBuffer> handle,
                                          const BufferAccess& access)
{
    buffers[handle] = access;
}

void
Cel::Renderer::ResourceTracker::set_state(const Handle<AllocatedImage> handle,
                                          const ImageAccess& access)
{
    images[handle] = access;
}

Cel::Renderer::BranchingResourceTracker
Cel::Renderer::BranchingResourceTracker::branch_off()
{
    return { *this };
}

void
Cel::Renderer::BranchingResourceTracker::compile(
    const ResourceTracker& original,
    ResourceTracker& writeTo)
{
    for (const auto& handle : original.buffers | std::views::keys) {
        writeTo.set_state(handle, state.get(handle));
    }
    for (const auto& handle : original.images | std::views::keys) {
        writeTo.set_state(handle, state.get(handle));
    }
}

Cel::Renderer::BranchingResourceTracker::BranchingResourceTracker(
    const ResourceTracker& tracker)
    : state(tracker.buffers, tracker.images)
{
}

Cel::Renderer::BranchingResourceTracker::BranchingResourceTracker(
    const BranchingResourceTracker& tracker) = default;
