#include "renderer/passes/HandleAllocator.h"

#include <mutex>
#include <utility>

static std::unordered_map<uint32_t, std::string> names{};
static uint32_t counter = 0;
static std::mutex mutex{};

Cel::Handle<Cel::Renderer::AllocatedBuffer>
Cel::Renderer::Passes::HandleAllocator::allocate_buffer(std::string name)
{
    std::lock_guard guard{ mutex };
    names[counter] = std::move(name);
    return { counter++ };
}

Cel::Handle<Cel::Renderer::AllocatedImage>
Cel::Renderer::Passes::HandleAllocator::allocate_image(std::string name)
{
    std::lock_guard guard{ mutex };
    names[counter] = std::move(name);
    return { counter++ };
}

Cel::Handle<Cel::Renderer::RenderGraph::RenderPass>
Cel::Renderer::Passes::HandleAllocator::allocate_pass(std::string name)
{
    std::lock_guard guard{ mutex };
    names[counter] = std::move(name);
    return { counter++ };
}

template<>
Cel::Handle<Cel::Renderer::AllocatedBuffer>
Cel::Renderer::Passes::HandleAllocator::allocate_handle<
    Cel::Renderer::AllocatedBuffer>(const std::string& name)
{
    return allocate_buffer(name);
}

template<>
Cel::Handle<Cel::Renderer::AllocatedImage>
Cel::Renderer::Passes::HandleAllocator::allocate_handle<
    Cel::Renderer::AllocatedImage>(const std::string& name)
{
    return allocate_image(name);
}

std::string
Cel::Renderer::Passes::HandleAllocator::get_name(
    const Handle<AllocatedBuffer> handle)
{
    return names.at(handle.index);
}

std::string
Cel::Renderer::Passes::HandleAllocator::get_name(
    const Handle<AllocatedImage> handle)
{
    return names.at(handle.index);
}

std::string
Cel::Renderer::Passes::HandleAllocator::get_name(
    const Handle<RenderGraph::RenderPass> handle)
{
    return names.at(handle.index);
}
