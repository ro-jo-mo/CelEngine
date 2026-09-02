#pragma once

#include "ResourceTracker.h"
#include "common/Handle.h"

#include <ranges>

namespace Cel::Renderer {

// Almighty tracker of all(?) allocated resources
// I suppose I should probably move resource ownership out of the asset server
// and to here
// For now the main purpose is for render passes
// Aliasing will only occur when requirements match perfectly. I'll allow the
// render graph to manually mark a resource as aliased
class VulkanResourceManager
{
  public:
    // At this stage only a handle is returned. No resource is actually
    // allocated
    Handle<AllocatedBuffer> get_handle_from_requirements(
        BufferRequirements requirements,
        const std::string& name);

    Handle<AllocatedImage> get_handle_from_requirements(
        const ImageRequirements& requirements,
        const std::string& name);

    // It's at this stage that the resource is actually created, assuming it
    // hasn't already
    AllocatedBuffer& get_resource_from_handle(Handle<AllocatedBuffer> handle,
                                              bool strictAliasing = false);

    AllocatedImage& get_resource_from_handle(Handle<AllocatedImage> handle,
                                             bool strictAliasing = false);

    BufferAccess get_resource_state(Handle<AllocatedBuffer> handle);

    ImageAccess get_resource_state(Handle<AllocatedImage> handle);

    bool does_resource_exist(Handle<AllocatedBuffer> handle);

    bool does_resource_exist(Handle<AllocatedImage> handle);

    /**
     * This resource is no longer in use and can be safely deleted
     */
    void free_resource(Handle<AllocatedBuffer> handle);
    void free_resource(Handle<AllocatedImage> handle);

    [[nodiscard]] BranchingResourceTracker branch_tracker() const;

  private:
    static bool is_compatible(const BufferRequirements& actual,
                              const BufferRequirements& requested);

    static bool is_compatible(const ImageRequirements& actual,
                              const ImageRequirements& requested);

    // Not happy with this naming. Not obvious that it takes zero ownership.
    [[nodiscard]] AllocatedBuffer allocate(
        Handle<AllocatedBuffer> handle) const;
    [[nodiscard]] AllocatedImage allocate(Handle<AllocatedImage> handle) const;

    void deallocate(const AllocatedBuffer& buffer) const;

    void deallocate(const AllocatedImage& image) const;

    template<typename Res, typename Req>
    class ResourcePool
    {
      public:
        explicit ResourcePool(VulkanResourceManager& manager)
            : manager(manager)
        {
        }

        Handle<Res> create_handle(Req req, const std::string& name);

        Res& get_or_allocate(Handle<Res> handle);

        void free(Handle<Res> handle);

        void flush();

      private:
        std::unordered_map<Handle<Res>, Req> requirements;
        // A handle is reusable only if it has been freed and the pool has been
        // flushed
        std::vector<Handle<Res>> reusableHandles;
        // A handle is freed when the user marks it as free
        std::vector<Handle<Res>> freed;

        std::unordered_map<Handle<Res>, Res> allocations;

        VulkanResourceManager& manager;

        friend class VulkanResourceManager;
    };

    ResourcePool<AllocatedBuffer, BufferRequirements> bufferPool;
    ResourcePool<AllocatedImage, ImageRequirements> imagePool;

    ResourceTracker tracker;

    VkDevice device;
    VmaAllocator allocator;
};

template<typename Res, typename Req>
Handle<Res>
VulkanResourceManager::ResourcePool<Res, Req>::create_handle(
    Req req,
    const std::string& name)
{
    Handle<Res> handle;
    if (reusableHandles.empty()) {
        handle = Passes::HandleAllocator::allocate_handle<Res>(name);
    } else {
        // pop back
        handle = reusableHandles.back();
        reusableHandles.pop_back();
    }

    requirements.emplace(handle, req);

    return handle;
}

template<typename Res, typename Req>
Res&
VulkanResourceManager::ResourcePool<Res, Req>::get_or_allocate(
    Handle<Res> handle)
{
    if (allocations.contains(handle)) {
        return allocations.at(handle);
    }

    // Attempt to alias TODO
    uint32_t bestFit = UINT32_MAX;

    for (const auto& freeRes : freed) {
        if (is_compatible(requirements.at(freeRes), requirements.at(handle))) {
            // Check if best fit, mark reused ...
        }
    }

    // Else allocate new
    allocations.emplace(handle, manager.allocate(handle));
    // Set state (if does not exist)
    manager.tracker.set_state(handle, {});

    return allocations.at(handle);
}

template<typename Res, typename Req>
void
VulkanResourceManager::ResourcePool<Res, Req>::free(Handle<Res> handle)
{
    freed.push_back(handle);
}

template<typename Res, typename Req>
void
VulkanResourceManager::ResourcePool<Res, Req>::flush()
{
    for (const auto& handle : freed) {
        if (!allocations.contains(handle)) {
            continue;
        }

        auto& res = allocations.at(handle);

        manager.deallocate(res);

        reusableHandles.push_back(handle);
        allocations.erase(handle);
    }

    freed.clear();
}

}