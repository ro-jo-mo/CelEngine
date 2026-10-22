#pragma once

#include "renderer/RenderTypes.h"

namespace Cel::Renderer {

class StagingPool
{
  public:
    Handle<AllocatedBuffer> get_handle();

    void reset();

  private:
    void allocate_handles();

    size_t upTo = 0;
    std::vector<Handle<AllocatedBuffer>> handles;
};

}