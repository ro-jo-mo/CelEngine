#pragma once

#include <string>
#include <unordered_map>

namespace Cel::Renderer {

// Design notes:

// After creating a resource, a handle can be returned
// Additionally, you can request a handle by the corresponding name

// Following islands design, a pointer to some arbitrary data can be passed to
// the draw function

// Creating resources can simply follow the same layout as the existing
// Utils::create_xx functions

// Buffers should be created per frame?
// Images can be created once?

// I'll also simplify the pipeline creation
// spirv reflect should be able to automate a large amount of it


class RenderGraph
{
  public:
    RenderGraph& add_pass();

  private:
    std::unordered_map<std::string, uint32_t> nameToIndex;

    friend class PassBuilder;
};
}