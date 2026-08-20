#pragma once

#include "common/Handle.h"
#include "renderer/VulkanTypes.h"

#include <functional>
#include <string>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {

struct BufferAccess
{
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
    uint32_t queue;
};

struct ImageAccess
{
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
    VkImageLayout layout;
    uint32_t queue;
};

struct BufferCreate
{
    Handle<AllocatedBuffer> id;
    BufferRequirements requirements;
};

struct ImageCreate
{
    Handle<AllocatedImage> id;
    ImageRequirements requirements;
};

struct BufferRead
{
    Handle<AllocatedBuffer> id;
    BufferAccess access;
};

struct BufferWrite
{
    Handle<AllocatedBuffer> id;
    BufferAccess access;
};

struct ImageRead
{
    Handle<AllocatedImage> id;
    ImageAccess access;
};

struct ImageWrite
{
    Handle<AllocatedImage> id;
    ImageAccess access;
};

// I'm storing queue on both the access and pass
// This is simply because
struct RenderPass
{
    Handle<RenderPass> id;

    uint32_t queue;

    std::vector<BufferCreate> newBuffers;
    std::vector<ImageCreate> newImages;

    std::vector<BufferRead> bufferReads;
    std::vector<ImageRead> imageReads;

    std::vector<BufferWrite> bufferWrites;
    std::vector<ImageWrite> imageWrites;
};

}