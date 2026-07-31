#pragma once

#include "RenderGraph.h"

#include <functional>
#include <string>

namespace Cel::Renderer {

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