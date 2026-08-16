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

Cel::Renderer::BufferAccess
Cel::Renderer::Resources::BranchingResourceTracker::get_state(
    Handle<AllocatedBuffer> buffer)
{
    if (buffers.contains(buffer)) {
        return buffers[buffer];
    }

    return original->get_state(buffer);
}

Cel::Renderer::ImageAccess
Cel::Renderer::Resources::BranchingResourceTracker::get_state(
    Handle<AllocatedImage> image)
{
    if (images.contains(image)) {
        return images[image];
    }

    return original->get_state(image);
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
    , original(tracker)
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

Cel::Renderer::Resources::BranchingResourceTracker::Branch::Branch(
    Branch* original)
    : original(original)
{
}

void
Cel::Renderer::Resources::BranchingResourceTracker::Branch::mark(
    const Handle<AllocatedBuffer> handle)
{
    buffers.insert(handle);
    antiBuffers.erase(handle);
}

void
Cel::Renderer::Resources::BranchingResourceTracker::Branch::mark(
    const Handle<AllocatedImage> handle)
{
    images.insert(handle);
    antiImages.erase(handle);
}

void
Cel::Renderer::Resources::BranchingResourceTracker::Branch::unmark(
    const Handle<AllocatedBuffer> handle)
{
    buffers.erase(handle);
    antiBuffers.insert(handle);
}
void
Cel::Renderer::Resources::BranchingResourceTracker::Branch::unmark(
    const Handle<AllocatedImage> handle)
{
    images.erase(handle);
    antiImages.insert(handle);
}

bool
Cel::Renderer::Resources::BranchingResourceTracker::Branch::is_marked(
    const Handle<AllocatedBuffer> handle) const
{
    if (buffers.contains(handle)) {
        return true;
    }
    if (antiBuffers.contains(handle)) {
        return false;
    }

    if (original == nullptr) {
        return false;
    }
    return original->is_marked(handle);
}

bool
Cel::Renderer::Resources::BranchingResourceTracker::Branch::is_marked(
    const Handle<AllocatedImage> handle) const
{
    if (images.contains(handle)) {
        return true;
    }
    if (antiImages.contains(handle)) {
        return false;
    }

    if (original == nullptr) {
        return false;
    }

    return original->is_marked(handle);
}
