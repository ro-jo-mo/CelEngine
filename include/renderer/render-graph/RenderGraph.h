#pragma once
#include <string>
#include <unordered_map>

namespace Cel::Renderer {

// RenderGraph
// A DAG of render passes
// Handles the insertion of barriers and transitions between passes

// RenderPass
// An individual pass in the render pipeline
// As a baseline, each pass

struct PassBuilder
{};

class RenderGraph
{
  public:
    RenderGraph& AddPass();

  private:
    std::unordered_map<std::string, uint32_t> nameToIndex;
};
}