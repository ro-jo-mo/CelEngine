#pragma once

#include "RenderPass.h"
#include "common/GrowVector.h"
#include "common/Handle.h"
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

// Each node in the graph is a renderpass
// Ordering is determined based on the input / outputs of the node
// Barriers are automatically inserted based on a nodes read / writes

class RenderGraph
{
  public:
    // When adding pass, we want to construct part of the dag if possible
    // In a way I want each alias of a single resource to have its own handle
    // But I also want a map that resolves these aliases to a single Image /
    // Buffer requirements struct
    void add_pass(RenderPass pass);

  private:
    Handle<AllocatedBuffer> get_buffer_handle_from_name(std::string name);
    Handle<AllocatedImage> get_image_handle_from_name(std::string name);

    // A resources requirements are defined once
    // by the passBuilder.create_xx stage
    // During compilation of the graph, the graph will be iterated in reverse
    // Each resource will be marked with the last stage it is used at
    // Then during a forward pass it can be decided to reuse a buffer / image
    // from another pass if it has no future use (reads? although a write with
    // no read after wards is always inreuse)
    Handle<AllocatedBuffer> create_buffer_or_reuse(
        std::string name,
        BufferRequirements requirements);

    Handle<AllocatedImage> create_image_or_reuse(
        std::string name,
        VkFormat format,
        ImageRequirements requirements);

    // Throughout the rendergraph resources will be written to, and adopt a new
    // name post write, which in turn implements the ordering of passes and
    // barriers As such multiple names map to the same resource

    // We want to firstly store a mapping of all names to a unique handle
    std::unordered_map<std::string, uint32_t> nameToIndex;
    // Then each handle  should have an index in the buffer / image requirements
    // Multiple handles will have the same requirements, as they're just aliases
    // of the same resource
    std::unordered_map<uint32_t, uint32_t> handleToAbsoluteIndex;
    // Helper to map handles back to names
    std::unordered_map<uint32_t, std::string> indexToName;

    // For each handle we also store the resources requirements
    Common::GrowVector<BufferRequirements> bufferRequirements;
    Common::GrowVector<ImageRequirements> imageRequirements;

    // Allocated vulkan resources
    std::array<Common::GrowVector<AllocatedBuffer>, FRAME_OVERLAP>
        perFrameBuffers;
    Common::GrowVector<AllocatedImage> images;

    VkDevice device;
    VmaAllocator allocator;

    friend class PassBuilder;
};

}