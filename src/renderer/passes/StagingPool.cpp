#include "renderer/passes/StagingPool.h"

#include "renderer/passes/HandleAllocator.h"

Cel::Handle<Cel::Renderer::AllocatedBuffer>
Cel::Renderer::StagingPool::get_handle()
{
    if (upTo >= handles.size()) {
        allocate_handles();
    }

    return handles[upTo++];
}

void
Cel::Renderer::StagingPool::reset()
{
    upTo = 0;
}

void
Cel::Renderer::StagingPool::allocate_handles()
{
    constexpr auto GROWTH = 32;

    handles.reserve(handles.size() + GROWTH);
    for (size_t i = 0; i < GROWTH; i++) {
        handles.emplace_back(
            Passes::HandleAllocator::allocate_buffer("staging_pool_buffer"));
    }
}
