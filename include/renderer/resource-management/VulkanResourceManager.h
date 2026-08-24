#pragma once

#include "ResourceTracker.h"
#include "common/Handle.h"

namespace Cel::Renderer {

// Almighty tracker of all(?) allocated resources
// I suppose I should probably move resource ownership out of the asset server
// and to here
// For now the main purpose is for render passes

class VulkanResourceManager
{
  public:
    // At this stage only a handle is returned. No resource is actually
    // allocated
    Handle<AllocatedBuffer> get_handle_from_requirements(
        BufferRequirements requirements);

    Handle<AllocatedImage> get_handle_from_requirements(
        ImageRequirements requirements);

    // It's at this stage that the resource is actually created
    AllocatedBuffer& get_resource_from_handle(Handle<AllocatedBuffer> handle);

    AllocatedImage& get_resource_from_handle(Handle<AllocatedImage> handle);

    BufferAccess get_buffer_state(Handle<AllocatedBuffer> buffer);

    ImageAccess get_image_state(Handle<AllocatedImage> image);

    /**
     * This resource is no longer in use and can be safely deleted
     */
    void free_resource(Handle<AllocatedBuffer> buffer);
    void free_resource(Handle<AllocatedImage> image);

    BranchingResourceTracker branch_tracker();

  private:
    ResourceTracker tracker;
};

}