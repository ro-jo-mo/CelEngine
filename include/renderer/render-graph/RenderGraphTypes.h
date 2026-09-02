#pragma once

#include "common/Handle.h"
#include "renderer/VulkanTypes.h"

#include <vulkan/vulkan_core.h>

namespace Cel::Renderer::RenderGraph {

struct BufferCreate
{
    Handle<AllocatedBuffer> id;
    bool perFrame;
    BufferRequirements requirements;
};

struct ImageCreate
{
    Handle<AllocatedImage> id;
    bool perFrame;
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
// This is for simplicity
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

// Replica of the actual vk struct
// Main thing is that the buffer / image link is replaced with a handle
struct BufferBarrier
{
    VkPipelineStageFlags2 srcStageMask;
    VkAccessFlags2 srcAccessMask;
    VkPipelineStageFlags2 dstStageMask;
    VkAccessFlags2 dstAccessMask;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
    Handle<AllocatedBuffer> buffer;
};
struct ImageBarrier
{
    VkPipelineStageFlags2 srcStageMask;
    VkAccessFlags2 srcAccessMask;
    VkPipelineStageFlags2 dstStageMask;
    VkAccessFlags2 dstAccessMask;
    VkImageLayout oldLayout;
    VkImageLayout newLayout;
    uint32_t srcQueueFamilyIndex;
    uint32_t dstQueueFamilyIndex;
    Handle<AllocatedImage> image;
};

// Each pass has an implicit signal semaphore, represented by a
// Handle<RenderPass>. If no pass waits on this semaphore, we can happily
// discard it. Otherwise, it needs to be created and signalled
// A barrier needs to be placed on both queues, but it can be derived from
// justis th The full barrier data can be derived from this
struct BufferTransfer
{
    Handle<RenderPass> semaphore;
    BufferBarrier barrier;
};
struct ImageTransfer
{
    Handle<RenderPass> semaphore;
    ImageBarrier barrier;
};

struct BufferMerge
{
    Handle<AllocatedBuffer> handle;
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
};
struct ImageMerge
{
    Handle<AllocatedImage> handle;
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
};

}