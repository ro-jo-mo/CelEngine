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

// Ideally the render graph will handle allocations of per frame resources
// i.e. buffers for indirect draws

// For now other buffers can be handled by the

class RenderGraph
{
  public:
    RenderGraph& add_pass();

  private:
    std::unordered_map<std::string, uint32_t> nameToIndex;

    friend class PassBuilder;
};
}