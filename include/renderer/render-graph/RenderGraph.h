#pragma once
#include <string>
#include <unordered_map>

namespace Cel::Renderer {

// What information does a renderpass need?

// Global vertex buffer, viewproj  matrix, material buffers

struct RenderPass
{};

class RenderGraph
{
  public:
    RenderPass& AddPass();

  private:
    std::unordered_map<std::string, uint32_t> nameToIndex;
};
}