#include "renderer/render-graph/PassServer.h"

#include "core/Config.h"
#include "core/Error.h"
#include "core/ThreadManager.h"
#include "renderer/AssetServer.h"
#include "renderer/Queues.h"
#include "renderer/SceneData.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/passes/Passes.h"
#include "renderer/resource-management/PipelineBuilder.h"

#include <ranges>

Cel::Renderer::RenderGraph::PassServer::PassServer(
    VkDevice device,
    const std::array<Queue, QUEUE_COUNT>& queues)
    : queues(queues)
    , device(device)
{
    // Create a mapping of the queues to an index
    for (const auto& [i, queue] : std::views::enumerate(queues)) {
        queueToIndex[queue.family] = i;
    }

    // Initialise pools
    size_t totalPools =
        FRAMES_IN_FLIGHT * QUEUE_COUNT * ThreadManager::total_threads();

    commandPools.reserve(totalPools);

    // Create in same way as we access
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {

        for (const auto& queue : queues) {
            VkCommandPoolCreateInfo create =
                Initialisers::command_pool_create_info(queue.family);

            for (uint32_t j = 0; j < ThreadManager::total_threads(); j++) {
                vkCreateCommandPool(
                    device, &create, nullptr, &commandPools.emplace_back());
            }
        }
    }

    // Initialise buffers
    availableBuffers.resize(totalPools);

    for (uint32_t i = 0; i < totalPools; i++) {
        allocate_cmd_buffers(i);
    }

    // Create semaphores

    VkSemaphoreTypeCreateInfo semaphoreType{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext = nullptr,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue = 0
    };
    VkSemaphoreCreateInfo semaphoreInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &semaphoreType,
        .flags = 0
    };

    for (const auto& queue : queues) {
        vkCreateSemaphore(device,
                          &semaphoreInfo,
                          nullptr,
                          &semaphores[queue.family].semaphore);
        semaphores[queue.family].current = 0;

        std::string name;
        if (queue.family == Queues::graphics.family) {
            name = "graphics_semaphore";
        } else if (queue.family == Queues::compute.family) {
            name = "compute_semaphore";
        } else {
            name = "transfer_semaphore";
        }

        Utils::set_resource_name(
            device,
            reinterpret_cast<uint64_t>(semaphores[queue.family].semaphore),
            VK_OBJECT_TYPE_SEMAPHORE,
            name.c_str());
    }

    VkFenceCreateInfo fenceInfo{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                 .pNext = nullptr,
                                 .flags = VK_FENCE_CREATE_SIGNALED_BIT };

    // Binary semaphores for acquisition
    semaphoreInfo.pNext = nullptr;

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        vkCreateFence(device, &fenceInfo, nullptr, &fences[i]);
        vkCreateSemaphore(
            device, &semaphoreInfo, nullptr, &acquireSemaphores[i]);

        auto name = fmt::format("acquire_semaphore_{}", i);

        Utils::set_resource_name(
            device,
            reinterpret_cast<uint64_t>(acquireSemaphores[i]),
            VK_OBJECT_TYPE_SEMAPHORE,
            name.c_str());
    }

    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
    };

    for (auto& allocator : descriptorAllocators) {
        allocator = { device, 512, sizes };
    }
}

VkCommandBuffer
Cel::Renderer::RenderGraph::PassServer::get_cmd_buffer(
    Handle<RenderPass> handle)
{
    // Cull unneeded passes
    if (!passesQueues.contains(handle)) {
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

    passCmdBuffers[currentFrame].emplace(handle, std::make_pair(cmd, index));

    // Begin command recording
    const VkCommandBufferBeginInfo beginInfo =
        Initialisers::command_buffer_begin_info(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkBeginCommandBuffer(cmd, &beginInfo);

    // Only bind descriptor for graphics & compute
    const auto queue = passesQueues.at(handle);
    if (!setupPasses.contains(handle) &&
        (queue == Queues::graphics.family || queue == Queues::compute.family)) {
        const auto bindPoint = queue == Queues::graphics.family
                                   ? VK_PIPELINE_BIND_POINT_GRAPHICS
                                   : VK_PIPELINE_BIND_POINT_COMPUTE;

        vkCmdBindDescriptorSets(cmd,
                                bindPoint,
                                PipelineBuilder::defaultPipelineLayout,
                                0,
                                1,
                                &baseDescriptorSet,
                                0,
                                nullptr);

        vkCmdBindIndexBuffer(cmd, indiceBuffer, 0, VK_INDEX_TYPE_UINT32);
    }

    Utils::set_resource_name(
        device,
        reinterpret_cast<uint64_t>(cmd),
        VK_OBJECT_TYPE_COMMAND_BUFFER,
        fmt::format("{}_{}",
                    Passes::HandleAllocator::get_name(handle).c_str(),
                    currentFrame)
            .c_str());

    return cmd;
}

Cel::Renderer::DescriptorAllocator&
Cel::Renderer::RenderGraph::PassServer::get_descriptor_allocator()
{
    return descriptorAllocators[currentFrame];
}

VkDevice
Cel::Renderer::RenderGraph::PassServer::get_device() const
{
    return device;
}

VkExtent2D
Cel::Renderer::RenderGraph::PassServer::get_extent() const
{
    return extent;
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
Cel::Renderer::RenderGraph::PassServer::release_resources(
    VulkanResourceManager& manager)
{
    currentFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;

    // We need to wait for the frame in flight to finish before resetting cmd
    // buffers
    vk_check(
        vkWaitForFences(device, 1, &fences[currentFrame], VK_TRUE, UINT64_MAX));
    vk_check(vkResetFences(device, 1, &fences[currentFrame]));

    get_descriptor_allocator().clear_pools();

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

    // Mark command buffers as available
    for (const auto& [cmd, i] :
         passCmdBuffers[currentFrame] | std::views::values) {

        availableBuffers[i].push_back(cmd);
    }
    passCmdBuffers[currentFrame].clear();

    for (const auto& queue : queues) {
        auto index = queueToIndex[queue.family] + QUEUE_COUNT * currentFrame;

        availableBuffers[queueToIndex[queue.family] *
                             ThreadManager::total_threads() +
                         currentFrame * QUEUE_COUNT *
                             ThreadManager::total_threads()]
            .append_range(prePostCommandBuffers[index]);

        prePostCommandBuffers[index].clear();
    }
}

void
Cel::Renderer::RenderGraph::PassServer::update_frame(
    const VkExtent2D _extent,
    const std::unordered_map<Handle<RenderPass>, RenderPass>& passesInUse,
    const std::unordered_map<Handle<AllocatedBuffer>, Handle<AllocatedBuffer>>&
        bufferMapping,
    const std::unordered_map<Handle<AllocatedImage>, Handle<AllocatedImage>>&
        imageMapping,
    const std::unordered_set<Handle<AllocatedBuffer>>& perFrameBuffers,
    const std::unordered_set<Handle<AllocatedImage>>& perFrameImages,
    const std::unordered_set<Handle<RenderPass>>& _setup_passes,
    VulkanResourceManager& manager,
    Assets::AssetServer& assetServer)
{
    extent = _extent;
    setupPasses = _setup_passes;
    passesQueues.clear();

    for (const auto& pass : passesInUse | std::views::values) {
        passesQueues.emplace(pass.id, pass.queue);
    }

    // Create a mapping from the pass handled to actual vk resources
    //
    // Set to be freed either the next frame (if transient resource) or the next
    // occurrence of this frame (if per frame)
    const auto nextFrame = (currentFrame + 1) % FRAMES_IN_FLIGHT;

    mappedBuffers.clear();
    mappedImages.clear();
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

    // Create the scene descriptor for this frame
    DescriptorWriter writer;

    auto textureCount =
        static_cast<uint32_t>(assetServer.textureCache.descriptors.size());
    {
        writer.write_buffer(0,
                            get_resource(Passes::sceneDataBuffer).buffer,
                            sizeof(Passes::SceneData::data),
                            0,
                            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        if (!assetServer.textureCache.descriptors.empty()) {
            VkWriteDescriptorSet arraySet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = textureCount,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = assetServer.textureCache.descriptors.data()
            };

            writer.write(arraySet);
        }
    }

    VkDescriptorSetVariableDescriptorCountAllocateInfo varInfo{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT,
        .pNext = nullptr,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &textureCount
    };

    baseDescriptorSet = get_descriptor_allocator().allocate(
        PipelineBuilder::defaultSetLayout, &varInfo);

    writer.update_set(device, baseDescriptorSet);

    indiceBuffer = assetServer.indiceBuffer.buffer.buffer;
}

VkCommandBuffer
Cel::Renderer::RenderGraph::PassServer::get_prepost_cmd_buffer(
    const uint32_t queue,
    const char* name)
{
    const auto index =
        queueToIndex[queue] * ThreadManager::total_threads() +
        currentFrame * QUEUE_COUNT * ThreadManager::total_threads();

    if (availableBuffers[index].empty()) {
        allocate_cmd_buffers(index);
    }

    const auto cmd = availableBuffers[index].back();

    availableBuffers[index].pop_back();

    prePostCommandBuffers[queueToIndex[queue] + QUEUE_COUNT * currentFrame]
        .push_back(cmd);

    Utils::set_resource_name(device,
                             reinterpret_cast<uint64_t>(cmd),
                             VK_OBJECT_TYPE_COMMAND_BUFFER,
                             fmt::format("{}_{}", name, currentFrame).c_str());

    return cmd;
}

Cel::Renderer::RenderGraph::PassServer::Semaphore&
Cel::Renderer::RenderGraph::PassServer::get_semaphore(const uint32_t queue)
{
    return semaphores[queue];
}

uint32_t
Cel::Renderer::RenderGraph::PassServer::get_pool_index(
    const Handle<RenderPass> handle)
{
    const auto queue = queueToIndex[passesQueues[handle]];
    const auto thread = ThreadManager::get_thread_id();

    auto index = currentFrame * (QUEUE_COUNT * ThreadManager::total_threads()) +
                 queue * (ThreadManager::total_threads()) + thread;

    return index;
}

void
Cel::Renderer::RenderGraph::PassServer::allocate_cmd_buffers(
    const uint32_t index)
{
    const auto& pool = commandPools[index];
    auto& buffers = availableBuffers[index];

    buffers.resize(CMD_BUFFERS_PER_POOL);

    const VkCommandBufferAllocateInfo allocInfo =
        Initialisers::command_buffer_allocate_info(pool, CMD_BUFFERS_PER_POOL);

    vkAllocateCommandBuffers(device, &allocInfo, buffers.data());
}
