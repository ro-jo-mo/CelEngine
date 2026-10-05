#include "renderer/resource-management/VulkanResourceManager.h"

#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"

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

Cel::Renderer::BufferAccess
Cel::Renderer::VulkanResourceManager::get_resource_state(
    const Handle<AllocatedBuffer> handle) const
{
    return tracker.get_state(handle);
}

Cel::Renderer::ImageAccess
Cel::Renderer::VulkanResourceManager::get_resource_state(
    Handle<AllocatedImage> handle) const
{
    return tracker.get_state(handle);
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
    return BranchingResourceTracker{ tracker };
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

    const VkImageCreateInfo img = Initialisers::image_create_info(
        requirements.format, requirements.usages, requirements.extent);

    VkImageViewCreateInfo view = Initialisers::image_view_create_info(
        requirements.format, nullptr, requirements.aspects);

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
        auto& [format, extent, usages, aspects] = get<1>(image);

        if (aspects == requirements.aspects && format == requirements.format &&
            usages == requirements.usages &&
            extent.width == requirements.extent.width &&
            extent.height == requirements.extent.height &&
            extent.depth == requirements.extent.depth) {

            return i;
        }
    }

    return UINT32_MAX;
}
