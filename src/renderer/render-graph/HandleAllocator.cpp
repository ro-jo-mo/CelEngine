#include <utility>

#include "renderer/render-graph/HandleAllocator.h"

static std::unordered_map<uint32_t, std::string> names;
static uint32_t counter;

Cel::Handle<Cel::Renderer::HandleAllocator::AllocatedBuffer>
Cel::Renderer::HandleAllocator::allocate_buffer(std::string name)
{
    names[counter] = std::move(name);
    return { counter++ };
}

Cel::Handle<Cel::Renderer::HandleAllocator::AllocatedBuffer>
Cel::Renderer::HandleAllocator::allocate_image(std::string name)
{
    names[counter] = std::move(name);
    return { counter++ };
}

Cel::Handle<Cel::Renderer::HandleAllocator::AllocatedBuffer>
Cel::Renderer::HandleAllocator::allocate_pass(std::string name)
{
    names[counter] = std::move(name);
    return { counter++ };
}
