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
    const BranchingResourceTracker& tracker)
    : reusable(tracker.reusable)
    , dirty(tracker.dirty)
    , state(tracker.state)
    , lastPassToAccessResource(tracker.lastPassToAccessResource)
{
}
