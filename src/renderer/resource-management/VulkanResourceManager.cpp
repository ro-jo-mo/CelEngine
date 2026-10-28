#include "renderer/resource-management/VulkanResourceManager.h"

#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"
#include "renderer/resource-management/ResourceTracker.h"

Cel::Renderer::VulkanResourceManager::VulkanResourceManager(
    VkDevice device,
    VmaAllocator allocator)
    : device(device)
    , allocator(allocator)
{
}

Cel::Handle<Cel::Renderer::AllocatedBuffer>
Cel::Renderer::VulkanResourceManager::get_handle_from_requirements(
    const BufferRequirements requirements,
    const std::string& name)
{
    const auto handle = bufferPool.create_handle(requirements, name);

    return handle;
}

Cel::Handle<Cel::Renderer::AllocatedImage>
Cel::Renderer::VulkanResourceManager::get_handle_from_requirements(
    const ImageRequirements& requirements,
    const std::string& name)
{
    const auto handle = imagePool.create_handle(requirements, name);

    return handle;
}

Cel::Renderer::AllocatedBuffer&
Cel::Renderer::VulkanResourceManager::get_resource_from_handle(
    const Handle<AllocatedBuffer> handle,
    bool strictAlias)
{
    return bufferPool.get_or_allocate(handle);
}

Cel::Renderer::AllocatedImage&
Cel::Renderer::VulkanResourceManager::get_resource_from_handle(
    const Handle<AllocatedImage> handle,
    bool strictAlias)
{
    return imagePool.get_or_allocate(handle);
}

const Cel::Renderer::BufferAccess&
Cel::Renderer::VulkanResourceManager::get_resource_state(
    const Handle<AllocatedBuffer> handle) const
{
    return bufferPool.states.at(handle);
}

const Cel::Renderer::ImageAccess&
Cel::Renderer::VulkanResourceManager::get_resource_state(
    Handle<AllocatedImage> handle) const
{
    return imagePool.states.at(handle);
}

bool
Cel::Renderer::VulkanResourceManager::does_resource_exist(
    Handle<AllocatedBuffer> handle) const
{
    // Has the handle been added to the pool and not been freed
    return bufferPool.requirements.contains(handle);
}

bool
Cel::Renderer::VulkanResourceManager::does_resource_exist(
    Handle<AllocatedImage> handle) const
{
    // Has the handle been added to the pool and not been freed
    return imagePool.requirements.contains(handle);
}

void
Cel::Renderer::VulkanResourceManager::free_resource(
    const Handle<AllocatedBuffer> handle)
{
    bufferPool.free(handle);
}

void
Cel::Renderer::VulkanResourceManager::free_resource(
    const Handle<AllocatedImage> handle)
{
    imagePool.free(handle);
}

Cel::Renderer::BranchingResourceTracker
Cel::Renderer::VulkanResourceManager::branch_tracker() const
{
    return BranchingResourceTracker{ *this };
}

void
Cel::Renderer::VulkanResourceManager::update(
    const BranchingResourceTracker& tracker)
{
    bufferPool = tracker.bufferPool;
    imagePool = tracker.imagePool;
}

Cel::Renderer::AllocatedBuffer
Cel::Renderer::VulkanResourceManager::allocate(
    const Handle<AllocatedBuffer> handle) const
{
    auto& requirements = bufferPool.requirements.at(handle);

    return Utils::create_buffer(
        handle,
        requirements.allocSize,
        requirements.usages,
        requirements.memoryUsage,
        Passes::HandleAllocator::get_name(handle).c_str(),
        device,
        allocator);
}

Cel::Renderer::AllocatedImage
Cel::Renderer::VulkanResourceManager::allocate(
    const Handle<AllocatedImage> handle) const
{
    auto& requirements = imagePool.requirements.at(handle);

    VkImageCreateInfo img = Initialisers::image_create_info(
        requirements.format, requirements.usages, requirements.extent);
    img.mipLevels = requirements.mipLevels;
    img.arrayLayers = requirements.arrayLayers;
    img.flags = requirements.flags;

    VkImageViewCreateInfo view = Initialisers::image_view_create_info(
        requirements.format, nullptr, requirements.aspects);
    view.viewType = requirements.viewType;
    view.subresourceRange.levelCount = requirements.mipLevels;
    view.subresourceRange.layerCount = requirements.arrayLayers;

    return Utils::create_image(
        handle,
        img,
        view,
        Passes::HandleAllocator::get_name(handle).c_str(),
        device,
        allocator);
}

void
Cel::Renderer::VulkanResourceManager::deallocate(
    const AllocatedBuffer& buffer) const
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

void
Cel::Renderer::VulkanResourceManager::deallocate(
    const AllocatedImage& image) const
{
    vmaDestroyImage(allocator, image.image, image.allocation);
    vkDestroyImageView(device, image.imageView, nullptr);
}

bool
Cel::Renderer::VulkanResourceManager::is_compatible(
    const BufferRequirements& actual,
    const BufferRequirements& requested)
{
    if (actual.allocSize < requested.allocSize) {
        return false;
    }

    // Tricky to compare
    if (actual.memoryUsage != requested.memoryUsage) {
        return false;
    }

    if (actual.usages != requested.usages) {
        return false;
    }

    return true;
}

bool
Cel::Renderer::VulkanResourceManager::is_compatible(
    const ImageRequirements& actual,
    const ImageRequirements& requested)
{
    if (actual.format != requested.format) {
        return false;
    }

    if (actual.usages != requested.usages) {
        return false;
    }

    if (actual.aspects != requested.aspects) {
        return false;
    }

    if (actual.mipLevels != requested.mipLevels ||
        actual.arrayLayers != requested.arrayLayers ||
        actual.flags != requested.flags ||
        actual.viewType != requested.viewType) {
        return false;
    }

    if (actual.extent.width < requested.extent.width ||
        actual.extent.height < requested.extent.height ||
        actual.extent.depth < requested.extent.depth) {
        return false;
    }

    return true;
}

uint32_t
Cel::Renderer::VulkanResourceManager::alias_resource(
    const BufferRequirements& requirements)
{
    for (const auto& [i, buffer] : std::views::enumerate(bufferPool.freed)) {
        auto& [allocSize, usages, memoryUsage] = get<1>(buffer);

        if (usages == requirements.usages &&
            allocSize == requirements.allocSize &&
            memoryUsage == requirements.memoryUsage) {

            return i;
        }
    }

    return UINT32_MAX;
}

uint32_t
Cel::Renderer::VulkanResourceManager::alias_resource(
    const ImageRequirements& requirements)
{
    for (const auto& [i, image] : std::views::enumerate(imagePool.freed)) {
        auto& freed = get<1>(image);

        if (freed.aspects == requirements.aspects &&
            freed.format == requirements.format &&
            freed.usages == requirements.usages &&
            freed.extent.width == requirements.extent.width &&
            freed.extent.height == requirements.extent.height &&
            freed.extent.depth == requirements.extent.depth &&
            freed.mipLevels == requirements.mipLevels &&
            freed.arrayLayers == requirements.arrayLayers &&
            freed.flags == requirements.flags &&
            freed.viewType == requirements.viewType) {

            return i;
        }
    }

    return UINT32_MAX;
}
