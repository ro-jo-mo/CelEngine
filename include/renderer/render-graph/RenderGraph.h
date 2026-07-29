#pragma once

#include "renderer/VulkanTypes.h"

#include <array>
#include <string>
#include <unordered_map>

namespace Cel::Renderer {

// Design notes:

// Need to move to a handle system for pipelines
// As the rendergraph is reconstructed each frame, we can use systems to add
// each pass As systems are used to add passes, we can additionally use the
// typical queries and resources as all other systems
// This allows us to use the same query system to for example filter only
// shadow casters

// At a minimum a pass needs to state which images and images it will read or
// write to

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
    std::string name;
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
    std::string name;
    std::string outName;
    VkAccessFlags2 access;
    VkPipelineStageFlags2 stages;
    VkImageLayout layout;
};

class RenderGraph
{
  public:
    RenderGraph& add_pass();

  private:
    Handle<AllocatedBuffer> get_buffer_handle_from_name(std::string name);
    Handle<AllocatedImage> get_image_handle_from_name(std::string name);

    // A resources requirements are defined once
    // by the passBuilder.create_xx stage
    // During compilation of the graph, the graph will be iterated in reverse
    // Each resource will be marked with the last stage it is used at
    // Then during a forward pass it can be decided to reuse a buffer / image
    // from another pass if it has no future use (reads? although a write with
    // no read after wards is always invalid)
    Handle<AllocatedBuffer> create_buffer_or_alias(
        std::string name,
        BufferRequirements requirements);

    Handle<AllocatedImage> create_image_or_alias(
        std::string name,
        VkFormat format,
        ImageRequirements requirements);

    // Throughout the rendergraph resources will be written to, and adopt a new
    // name post write, which in turn implements the ordering of passes and
    // barriers As such multiple names map to the same resource
    std::unordered_map<std::string, uint32_t> nameToIndex;

    // For each handle we also store the resources requirements
    std::vector<BufferRequirements> bufferRequirements;
    std::vector<ImageRequirements> imageRequirements;

    // Allocated vulkan resources
    std::array<std::vector<AllocatedBuffer>, FRAME_OVERLAP> perFrameBuffers;
    std::vector<AllocatedImage> images;

    VkDevice device;
    VmaAllocator allocator;

    friend class PassBuilder;
};
}