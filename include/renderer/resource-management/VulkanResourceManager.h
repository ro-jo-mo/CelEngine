#pragma once

#include "ResourceTracker.h"
#include "common/Handle.h"

#include <ranges>

namespace Cel::Renderer {
namespace RenderGraph {
class Graph;
}

// Almighty tracker of all(?) allocated resources
// I suppose I should probably move resource ownership out of the asset server
// and to here
// For now the main purpose is for render passes
// Aliasing will only occur when requirements match perfectly. I'll allow the
// render graph to manually mark a resource as aliased
class VulkanResourceManager
{
  public:
    VulkanResourceManager(VkDevice device, VmaAllocator allocator);

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

    BufferAccess get_resource_state(Handle<AllocatedBuffer> handle) const;

    ImageAccess get_resource_state(Handle<AllocatedImage> handle) const;

    [[nodiscard]] bool does_resource_exist(
        Handle<AllocatedBuffer> handle) const;

    [[nodiscard]] bool does_resource_exist(Handle<AllocatedImage> handle) const;

    /**
     * This resource is no longer in use and can be safely deleted
     */
    void free_resource(Handle<AllocatedBuffer> handle);
    void free_resource(Handle<AllocatedImage> handle);

    [[nodiscard]] BranchingResourceTracker branch_tracker() const;

    VkDevice device;

  private:
    static bool is_compatible(const BufferRequirements& actual,
                              const BufferRequirements& requested);

    static bool is_compatible(const ImageRequirements& actual,
                              const ImageRequirements& requested);

    // Return an index to the best match for these requirements
    // UINT32_MAX means no match
    uint32_t alias_resource(const BufferRequirements& requirements);
    uint32_t alias_resource(const ImageRequirements& requirements);

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

        Res& alias(Handle<Res> handle, uint32_t index);

        void free(Handle<Res> handle);

        void flush();

      private:
        std::unordered_map<Handle<Res>, Req> requirements;
        // We reuse handles that have been freed
        std::vector<Handle<Res>> reusableHandles;

        // A handle is freed when the user marks it as free
        // Then the allocated resource is added to "freed
        std::vector<std::tuple<Res, Req, typename Req::Access>> freed;

        std::unordered_map<Handle<Res>, Res> allocations;

        VulkanResourceManager& manager;

        friend class VulkanResourceManager;
    };

    ResourcePool<AllocatedBuffer, BufferRequirements> bufferPool{ *this };
    ResourcePool<AllocatedImage, ImageRequirements> imagePool{ *this };

    ResourceTracker tracker;

    VmaAllocator allocator;

    friend class RenderGraph::Graph;
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
    manager.tracker.set_state(handle, typename Req::Access());

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

    auto index = manager.alias_resource(requirements.at(handle));

    if (index != UINT32_MAX) {
        return alias(handle, index);
    }

    // Else allocate new
    allocations.emplace(handle, manager.allocate(handle));

    // Set state to none
    manager.tracker.set_state(handle, {});

    return allocations.at(handle);
}

template<typename Res, typename Req>
Res&
VulkanResourceManager::ResourcePool<Res, Req>::alias(Handle<Res> handle,
                                                     uint32_t index)
{
    auto& tuple = freed[index];

    auto& res = (*allocations.emplace(handle, get<0>(tuple)).first).second;

    manager.tracker.set_state(handle, get<2>(tuple));

    freed.erase(freed.begin() + index);

    return res;
}

template<typename Res, typename Req>
void
VulkanResourceManager::ResourcePool<Res, Req>::free(Handle<Res> handle)
{
    freed.emplace_back(allocations.at(handle),
                       requirements.at(handle),
                       manager.tracker.get_state(handle));

    reusableHandles.push_back(handle);
    requirements.erase(handle);
    allocations.erase(handle);
}

template<typename Res, typename Req>
void
VulkanResourceManager::ResourcePool<Res, Req>::flush()
{
    for (const auto& res : freed) {
        manager.deallocate(res);
    }

    freed.clear();
}

}