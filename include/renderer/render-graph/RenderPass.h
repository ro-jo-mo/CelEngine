#pragma once

#include "common/Handle.h"
#include "renderer/VulkanTypes.h"

#include <functional>
#include <string>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {

template<typename Name>
struct BufferCreate
{
    Name name;
    BufferRequirements requirements;
};

template<typename Name>
struct ImageCreate
{
    Name name;
    ImageRequirements requirements;
};

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
struct BufferRead
{
    Name name;
    BufferAccess access;
};

template<typename Name>
struct BufferWrite
{
    Name inName;
    Name outName;
    BufferAccess access;
};

template<typename Name>
struct ImageRead
{
    Name name;
    ImageAccess access;
};

template<typename Name>
struct ImageWrite
{
    Name inName;
    Name outName;
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

struct RenderPassCompiled
{
    std::function<void(VkCommandBuffer, void*)> execute;

    std::vector<BufferCreate<Handle<AllocatedBuffer>>> newBuffers;
    std::vector<ImageCreate<Handle<AllocatedImage>>> newImages;

    std::vector<BufferRead<Handle<AllocatedBuffer>>> bufferReads;
    std::vector<ImageRead<Handle<AllocatedImage>>> imageReads;

    std::vector<BufferWrite<Handle<AllocatedBuffer>>> bufferWrites;
    std::vector<ImageWrite<Handle<AllocatedImage>>> imageWrites;
};

}