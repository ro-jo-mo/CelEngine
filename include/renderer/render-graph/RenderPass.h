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
    VkDeviceSize offset;
    VkDeviceSize size;
    uint32_t queue;
};

struct ImageAccess
{
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
    VkImageLayout layout;
    uint32_t queue;
};

template<typename Name>
struct BufferCreate
{
    Name id;
    BufferRequirements requirements;
};

template<typename Name>
struct ImageCreate
{
    Name id;
    ImageRequirements requirements;
};

template<typename Name>
struct BufferRead
{
    Name id;
    BufferAccess access;
};

template<typename Name>
struct BufferWrite
{
    Name inId;
    Name outId;
    BufferAccess access;
};

template<typename Name>
struct ImageRead
{
    Name id;
    ImageAccess access;
};

template<typename Name>
struct ImageWrite
{
    Name inId;
    Name outId;
    ImageAccess access;
};

struct RenderPass
{
    std::string name;

    std::function<void(VkCommandBuffer, void*)> execute;

    std::vector<BufferCreate<std::string>> newBuffers;
    std::vector<ImageCreate<std::string>> newImages;

    std::vector<BufferRead<std::string>> bufferReads;
    std::vector<ImageRead<std::string>> imageReads;

    std::vector<BufferWrite<std::string>> bufferWrites;
    std::vector<ImageWrite<std::string>> imageWrites;
};

using BufferReadC = BufferRead<Handle<AllocatedBuffer>>;
using BufferWriteC = BufferWrite<Handle<AllocatedBuffer>>;
using ImageReadC = ImageRead<Handle<AllocatedImage>>;
using ImageWriteC = ImageWrite<Handle<AllocatedImage>>;

struct RenderPassCompiled
{
    std::function<void(VkCommandBuffer, void*)> execute;

    std::vector<BufferCreate<Handle<AllocatedBuffer>>> newBuffers;
    std::vector<ImageCreate<Handle<AllocatedImage>>> newImages;

    std::vector<BufferReadC> bufferReads;
    std::vector<ImageReadC> imageReads;

    std::vector<BufferWriteC> bufferWrites;
    std::vector<ImageWriteC> imageWrites;
};

}