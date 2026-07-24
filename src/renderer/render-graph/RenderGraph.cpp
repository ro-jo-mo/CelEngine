#include "renderer/render-graph/RenderGraph.h"

#include <ranges>

using namespace Cel::Renderer;

Handle<AllocatedBuffer>
RenderGraph::get_buffer_handle_from_name(std::string name)
{
    // As render passes may be added out of order, I don't guarantee the handle
    // will exist yet
    if (!nameToIndex.contains(name)) {
        auto size = perFrameBuffers[0].size();
        nameToIndex.emplace(name, size);
        for (auto& list : perFrameBuffers) {
            list.resize(size + 1);
        }
    }

    return { nameToIndex[name] };
}

Handle<AllocatedImage>
RenderGraph::get_image_handle_from_name(std::string name)
{
    // As render passes may be added out of order, I don't guarantee the handle
    // will exist yet
    if (!nameToIndex.contains(name)) {
        nameToIndex.emplace(name, images.size());
        images.resize(images.size() + 1);
    }

    return { nameToIndex[name] };
}

Handle<AllocatedBuffer>
RenderGraph::add_buffer(std::string name,
                        std::array<AllocatedBuffer, FRAME_OVERLAP> buffers)
{
    auto handle = get_buffer_handle_from_name(name);

    for (const auto& [list, buffer] :
         std::views::zip(perFrameBuffers, buffers)) {
        list[handle.index] = buffer;
    }

    return handle;
}

Handle<AllocatedImage>
RenderGraph::add_image(std::string name, AllocatedImage image)
{
    auto handle = get_image_handle_from_name(name);

    images[handle.index] = image;

    return handle;
}
