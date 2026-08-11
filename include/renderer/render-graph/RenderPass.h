#pragma once

#include <functional>
#include <string>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {

struct BufferRequirements
{
    std::string name;
    size_t allocSize;
    VkBufferUsageFlags usages;
    VmaMemoryUsage memoryUsage;
};

struct ImageRequirements
{
    std::string name;
    VkFormat format;
    VkExtent3D extent;
    VkImageUsageFlags usages;
    VkImageAspectFlags aspects;
};

struct BufferRead
{
    std::string name;
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
    VkDeviceSize offset;
    VkDeviceSize size;
};

struct BufferWrite
{
    std::string inName;
    std::string outName;
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
    VkDeviceSize offset;
    VkDeviceSize size;
};

struct ImageRead
{
    std::string name;
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
    VkImageLayout layout;
};

struct ImageWrite
{
    std::string inName;
    std::string outName;
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
    VkImageLayout layout;
};

struct RenderPass
{
    std::string name;

    std::function<void(void*)> execute;

    std::vector<BufferRequirements> newBuffers;
    std::vector<ImageRequirements> newImages;

    std::vector<BufferRead> bufferReads;
    std::vector<ImageRead> imageReads;

    std::vector<BufferWrite> bufferWrites;
    std::vector<ImageWrite> imageWrites;
};

}