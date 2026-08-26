#include <utility>

#include "renderer/passes/HandleAllocator.h"

static std::unordered_map<uint32_t, std::string> names;
static uint32_t counter;

Cel::Handle<Cel::Renderer::AllocatedBuffer>
Cel::Renderer::Passes::HandleAllocator::allocate_buffer(std::string name)
{
    names[counter] = std::move(name);
    return { counter++ };
}

Cel::Handle<Cel::Renderer::AllocatedImage>
Cel::Renderer::Passes::HandleAllocator::allocate_image(std::string name)
{
    names[counter] = std::move(name);
    return { counter++ };
}

Cel::Handle<Cel::Renderer::RenderGraph::RenderPass>
Cel::Renderer::Passes::HandleAllocator::allocate_pass(std::string name)
{
    names[counter] = std::move(name);
    return { counter++ };
}

std::string
Cel::Renderer::Passes::HandleAllocator::get_name(
    const Handle<AllocatedBuffer> handle)
{
    return names[handle.index];
}

std::string
Cel::Renderer::Passes::HandleAllocator::get_name(
    const Handle<AllocatedImage> handle)
{
    return names[handle.index];
}

std::string
Cel::Renderer::Passes::HandleAllocator::get_name(
    const Handle<RenderGraph::RenderPass> handle)
{
    return names[handle.index];
}
