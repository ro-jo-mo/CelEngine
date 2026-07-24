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

class RenderGraph
{
  public:
    RenderGraph& add_pass();
    Handle<AllocatedBuffer> get_buffer_handle_from_name(std::string name);
    Handle<AllocatedImage> get_image_handle_from_name(std::string name);

  private:
    Handle<AllocatedBuffer> add_buffer(
        std::string name,
        std::array<AllocatedBuffer, FRAME_OVERLAP> buffers);
    Handle<AllocatedImage> add_image(std::string name, AllocatedImage image);

    std::unordered_map<std::string, uint32_t> nameToIndex;
    std::array<std::vector<AllocatedBuffer>, FRAME_OVERLAP> perFrameBuffers;
    std::vector<AllocatedImage> images;

    friend class PassBuilder;
};
}