#include "renderer/render-graph/PassBuilder.h"

#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"

using namespace Cel::Renderer;

Handle<AllocatedBuffer>
PassBuilder::create_buffer(const std::string& name,
                           const size_t allocSize,
                           VkBufferUsageFlags usages,
                           VmaMemoryUsage memoryUsage) const
{
    // Create per frame
    // Return a single handle
    Handle<AllocatedBuffer> handle{};

    for (size_t i = 0; i < FRAME_OVERLAP; i++) {
        handle = graph.add_buffer(
            name,
            Utils::create_buffer(
                allocSize, usages, memoryUsage, name.c_str(), allocator));
    }

    // Lazy approach, don't feel like passing the params to render graph
    handle.index -= FRAME_OVERLAP - 1;

    return handle;
}

Handle<AllocatedImage>
PassBuilder::create_image(const std::string& name,
                          const VkFormat format,
                          const VkExtent3D extent,
                          const VkImageUsageFlags usages,
                          const VkImageAspectFlags aspects) const
{
    const VkImageCreateInfo imageInfo =
        Initialisers::image_create_info(format, usages, extent);

    VmaAllocationCreateInfo allocInfo{
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };

    AllocatedImage image{};

    vmaCreateImage(allocator,
                   &imageInfo,
                   &allocInfo,
                   &image.image,
                   &image.allocation,
                   nullptr);
    vmaSetAllocationName(allocator, image.allocation, name.c_str());

    const VkImageViewCreateInfo viewInfo =
        Initialisers::image_view_create_info(format, image.image, aspects);

    vk_check(vkCreateImageView(device, &viewInfo, nullptr, &image.imageView));

    return graph.add_image(name, image);
}
