#include "renderer/render-graph/PassServer.h"

#include "core/Config.h"
#include "core/Error.h"
#include "core/ThreadManager.h"
#include "renderer/VulkanHelpers.h"

#include <ranges>

Cel::Renderer::RenderGraph::PassServer::PassServer(
    VkDevice device,
    std::vector<uint32_t>& queues)
    : device(device)
{
    // Sanity check
    assert(QUEUE_COUNT == queues.size());

    // Create a mapping of the queues to an index
    for (const auto& [i, queue] : std::views::enumerate(queues)) {
        queueToIndex[queue] = i;
    }

    // Initialise pools
    size_t totalPools =
        FRAMES_IN_FLIGHT * QUEUE_COUNT * ThreadManager::total_threads();

    commandPools.reserve(totalPools);

    // Create in same way as we access
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {

        for (const auto& queue : queues) {
            VkCommandPoolCreateInfo create =
                Initialisers::command_pool_create_info(queue);

            for (uint32_t j = 0; j < ThreadManager::total_threads(); j++) {
                VkCommandPool pool;

                vkCreateCommandPool(device, &create, nullptr, &pool);

                commandPools.emplace_back(pool);
            }
        }
    }

    // Initialise buffers
    availableBuffers.resize(totalPools);

    for (uint32_t i = 0; i < totalPools; i++) {
        allocate_cmd_buffers(i);
    }
}

VkCommandBuffer
Cel::Renderer::RenderGraph::PassServer::get_cmd_buffer(
    Handle<RenderPass> handle)
{
    // Cull unneeded passes
    if (!validPasses.contains(handle)) {
        return VK_NULL_HANDLE;
    }

    // This should only be the case during render graph execution, it is not
    // thread safe as we can't guarantee it's on the correct thread, as such
    // throw an error Render graph should access it properly
    if (passCmdBuffers[currentFrame].contains(handle)) {
        throw_error("This pass ({}) already has a cmd buffer assigned. Cannot "
                    "assign another buffer",
                    Passes::HandleAllocator::get_name(handle));
    }

    auto index = get_pool_index(handle);

    if (availableBuffers[index].empty()) {
        allocate_cmd_buffers(index);
    }

    // pop back
    auto cmd = availableBuffers[index].back();
    availableBuffers[index].pop_back();

    passCmdBuffers[currentFrame].emplace(handle, cmd);

    return cmd;
}

Cel::Renderer::AllocatedBuffer&
Cel::Renderer::RenderGraph::PassServer::get_resource(
    const Handle<AllocatedBuffer> handle) const
{

    return mappedBuffers.at(handle);
}

Cel::Renderer::AllocatedImage&
Cel::Renderer::RenderGraph::PassServer::get_resource(
    const Handle<AllocatedImage> handle) const
{
    return mappedImages.at(handle);
}

void
Cel::Renderer::RenderGraph::PassServer::update_frame(
    const uint32_t _currentFrame,
    const std::unordered_map<Handle<RenderPass>, uint32_t>& _validPasses,
    const std::unordered_map<Handle<AllocatedBuffer>, Handle<AllocatedBuffer>>&
        bufferMapping,
    const std::unordered_map<Handle<AllocatedImage>, Handle<AllocatedImage>>&
        imageMapping,
    const std::unordered_set<Handle<AllocatedBuffer>>& perFrameBuffers,
    const std::unordered_set<Handle<AllocatedImage>>& perFrameImages,
    VulkanResourceManager& manager)
{
    currentFrame = _currentFrame;
    validPasses = _validPasses;

    // Free buffers + images from the last time
    // Freeing at this point allows the resource manager to possibly reuse it
    for (const auto& handle : buffersToFree[currentFrame]) {
        manager.free_resource(handle);
    }
    for (const auto& handle : imagesToFree[currentFrame]) {
        manager.free_resource(handle);
    }
    buffersToFree[currentFrame].clear();
    imagesToFree[currentFrame].clear();

    // Reset command pools for this frame
    const size_t width = QUEUE_COUNT * ThreadManager::total_threads();
    for (size_t i = width * currentFrame; i < width * (currentFrame + 1); i++) {
        vkResetCommandPool(device, commandPools[i], 0);
    }

    // Create a mapping from the pass handled to actual vk resources
    // Set to be freed either the next frame (if transient resource) or the next
    // occurrence of this frame (if per frame)
    const auto nextFrame = currentFrame % FRAMES_IN_FLIGHT;

    for (const auto& [handle, mapped] : bufferMapping) {
        const auto freeFrame =
            perFrameBuffers.contains(handle) ? currentFrame : nextFrame;

        buffersToFree[freeFrame].emplace_back(mapped);
        mappedBuffers.emplace(handle, manager.get_resource_from_handle(mapped));
    }
    for (const auto& [handle, mapped] : imageMapping) {
        const auto freeFrame =
            perFrameImages.contains(handle) ? currentFrame : nextFrame;

        imagesToFree[freeFrame].emplace_back(mapped);
        mappedImages.emplace(handle, manager.get_resource_from_handle(mapped));
    }
}

uint32_t
Cel::Renderer::RenderGraph::PassServer::get_pool_index(
    const Handle<RenderPass> handle)
{
    const auto queue = queueToIndex[validPasses[handle]];
    const auto thread = ThreadManager::get_thread_id();

    return currentFrame * (QUEUE_COUNT * ThreadManager::total_threads()) +
           queue * (ThreadManager::total_threads()) + thread;
}

void
Cel::Renderer::RenderGraph::PassServer::allocate_cmd_buffers(
    const uint32_t index)
{
    const auto& pool = commandPools[index];
    auto& buffers = availableBuffers[index];

    buffers.resize(CMD_BUFFERS_PER_POOL + buffers.size());

    const VkCommandBufferAllocateInfo allocInfo =
        Initialisers::command_buffer_allocate_info(pool, CMD_BUFFERS_PER_POOL);

    vkAllocateCommandBuffers(device, &allocInfo, buffers.data());
}
