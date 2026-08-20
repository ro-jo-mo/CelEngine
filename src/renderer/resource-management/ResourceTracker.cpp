#include "renderer/resource-management/ResourceTracker.h"

Cel::Renderer::BufferAccess
Cel::Renderer::Resources::ResourceTracker::get_state(
    Handle<AllocatedBuffer> buffer)
{
    return buffers[buffer];
}

Cel::Renderer::ImageAccess
Cel::Renderer::Resources::ResourceTracker::get_state(
    Handle<AllocatedImage> image)
{
    return images[image];
}

Cel::Renderer::Resources::BranchingResourceTracker
Cel::Renderer::Resources::BranchingResourceTracker::branch_off()
{
    return BranchingResourceTracker(this);
}

Cel::Renderer::Resources::BranchingResourceTracker::BranchingResourceTracker(
    ResourceTracker* tracker)
    : reusable(nullptr)
    , dirty(nullptr)
    , read(nullptr)

{
}

Cel::Renderer::Resources::BranchingResourceTracker::BranchingResourceTracker(
    BranchingResourceTracker* tracker)
    : reusable(tracker->reusable)
    , dirty(tracker->dirty)
    , read(tracker->read)
    , original(tracker)
{
}
