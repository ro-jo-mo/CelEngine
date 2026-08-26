#include "renderer/resource-management/VulkanResourceManager.h"

#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"

Cel::Handle<Cel::Renderer::AllocatedBuffer>
Cel::Renderer::VulkanResourceManager::get_handle_from_requirements(
    const BufferRequirements requirements)
{
    return bufferPool.create_handle(requirements);
}

Cel::Handle<Cel::Renderer::AllocatedImage>
Cel::Renderer::VulkanResourceManager::get_handle_from_requirements(
    const ImageRequirements& requirements)
{
    return imagePool.create_handle(requirements);
}

Cel::Renderer::AllocatedBuffer&
Cel::Renderer::VulkanResourceManager::get_resource_from_handle(
    const Handle<AllocatedBuffer> handle)
{
    return bufferPool.get_or_allocate(handle);
}

Cel::Renderer::AllocatedImage&
Cel::Renderer::VulkanResourceManager::get_resource_from_handle(
    const Handle<AllocatedImage> handle)
{
    return imagePool.get_or_allocate(handle);
}

Cel::Renderer::BufferAccess
Cel::Renderer::VulkanResourceManager::get_buffer_state(
    const Handle<AllocatedBuffer> handle)
{
    return tracker.get_state(handle);
}

Cel::Renderer::ImageAccess
Cel::Renderer::VulkanResourceManager::get_image_state(
    Handle<AllocatedImage> handle)
{
    return tracker.get_state(handle);
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
    const BufferRequirements& requirements) const
{
    return Utils::create_buffer(requirements.allocSize,
                                requirements.usages,
                                requirements.memoryUsage,
                                "buffer_alloc",
                                allocator);
}

Cel::Renderer::AllocatedImage
Cel::Renderer::VulkanResourceManager::allocate(
    const ImageRequirements& requirements)
{

    const VkImageCreateInfo img = Initialisers::image_create_info(
        requirements.format, requirements.usages, requirements.extent);

    const VkImageViewCreateInfo view = Initialisers::image_view_create_info(
        requirements.format, nullptr, requirements.aspects);

    return Utils::create_image(img, view, "img_alloc", device, allocator);
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
