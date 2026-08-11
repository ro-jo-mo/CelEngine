#include "renderer/render-graph/RenderGraph.h"

#include <ranges>

using namespace Cel::Renderer;

void
RenderGraph::add_pass(RenderPass pass)
{
    // Add aliases

    // iter creates
    for (auto& buff : pass.newBuffers) {
        // index to name name to index abs blah blah
        bufferRequirements.push_back(buff);
    }
    for (auto& img : pass.newImages) {
        imageRequirements.push_back(img);
    }

    // iter writes
    for (auto& buff : pass.bufferWrites) {
    }
    for (auto& img : pass.imageWrites) {
    }

    // add pass to dag
}

Cel::Handle<AllocatedBuffer>
RenderGraph::get_buffer_handle_from_name(std::string name)
{
    // As render passes may be added out of order, I don't guarantee the handle
    // will exist yet
    if (!nameToIndex.contains(name)) {
        auto size = perFrameBuffers[0].size();
        nameToIndex.emplace(name, size);
        for (auto& list : perFrameBuffers) {
        }
    }

    return { nameToIndex[name] };
}

Cel::Handle<AllocatedImage>
RenderGraph::get_image_handle_from_name(std::string name)
{
    // As render passes may be added out of order, I don't guarantee the handle
    // will exist yet
    if (!nameToIndex.contains(name)) {
        nameToIndex.emplace(name, images.size());
    }

    return { nameToIndex[name] };
}
