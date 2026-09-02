#include "renderer/resource-management/ResourceTracker.h"

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
    return BranchingResourceTracker(*this);
}

Cel::Renderer::BranchingResourceTracker::BranchingResourceTracker(
    const ResourceTracker& tracker)
    : reusable(nullptr)
    , dirty(nullptr)
    , state(tracker.buffers, tracker.images)
    , lastPassToAccessResource(nullptr)

{
}

Cel::Renderer::BranchingResourceTracker::BranchingResourceTracker(
    const BranchingResourceTracker& tracker) = default;
