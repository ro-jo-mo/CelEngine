#include "renderer/passes/HandleAllocator.h"

#include <fmt/base.h>
#include <mutex>
#include <utility>

namespace {
// It's completely insane that I need to wrap basic variables in functions like
// this. A reasonable language would just guarantee the value is initialised
// before any accesses.

uint32_t&
pass_counter()
{
    static uint32_t counter = 0;
    return counter;
}

uint32_t&
buffer_counter()
{
    static uint32_t counter = 0;
    return counter;
}

uint32_t&
image_counter()
{
    static uint32_t counter = 0;
    return counter;
}

std::mutex&
mutex()
{
    static std::mutex mutex{};
    return mutex;
}

}

Cel::Handle<Cel::Renderer::RenderGraph::RenderPass>
Cel::Renderer::Passes::HandleAllocator::allocate_pass(std::string name)
{
    std::lock_guard guard{ mutex() };
    _passNames.emplace(pass_counter(), std::move(name));
    return { pass_counter()++ };
}

Cel::Handle<Cel::Renderer::AllocatedBuffer>
Cel::Renderer::Passes::HandleAllocator::allocate_buffer(std::string name)
{
    std::lock_guard guard{ mutex() };
    _bufferNames.emplace(buffer_counter(), std::move(name));
    return { buffer_counter()++ };
}

Cel::Handle<Cel::Renderer::AllocatedImage>
Cel::Renderer::Passes::HandleAllocator::allocate_image(std::string name)
{
    std::lock_guard guard{ mutex() };
    _imageNames.emplace(image_counter(), std::move(name));
    return { image_counter()++ };
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
    const Handle<RenderGraph::RenderPass> handle)
{
    return _passNames.at(handle.index);
}

std::string
Cel::Renderer::Passes::HandleAllocator::get_name(
    const Handle<AllocatedBuffer> handle)
{
    return _bufferNames.at(handle.index);
}

std::string
Cel::Renderer::Passes::HandleAllocator::get_name(
    const Handle<AllocatedImage> handle)
{
    return _imageNames.at(handle.index);
}
