#include "renderer/render-graph/ExecutionPlan.h"

#include "core/Error.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/render-graph/PassServer.h"

using namespace Cel::Renderer;
using namespace Cel::Renderer::RenderGraph;

ExecutionPlan
ExecutionPlan::branch_off()
{
    return ExecutionPlan(this);
}

void
ExecutionPlan::push(const ExecutePass& exec)
{
    execution.push_back(exec);

    // Rough cost estimate
    totalCost +=
        (exec.bufferTransfers.size() + exec.imageTransfers.size()) * 2 +
        exec.bufferBarriers.size() + exec.imageBarriers.size();
}

uint32_t
ExecutionPlan::cost() const
{
    return totalCost;
}

std::vector<ExecutionPlan::ExecutePass>
ExecutionPlan::compile()
{
    std::vector<ExecutePass> compiled;

    // Append in order
    add_execution_to_list(this, compiled);

    return compiled;
}

VkBufferMemoryBarrier2
create_barrier(const BufferBarrier& barrier, VulkanResourceManager& manager)
{
    return { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
             .pNext = nullptr,
             .srcStageMask = barrier.srcStageMask,
             .srcAccessMask = barrier.srcAccessMask,
             .dstStageMask = barrier.dstStageMask,
             .dstAccessMask = barrier.dstAccessMask,
             .srcQueueFamilyIndex = barrier.srcQueueFamilyIndex,
             .dstQueueFamilyIndex = barrier.dstQueueFamilyIndex,
             .buffer = manager.get_resource_from_handle(barrier.handle).buffer,
             .offset = 0,
             .size = VK_WHOLE_SIZE };
}

VkImageAspectFlags
format_to_image_aspect(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
        case VK_FORMAT_R8G8B8_UNORM:
            return VK_IMAGE_ASPECT_COLOR_BIT;

        default:
            Cel::throw_error("unimplemented format {}",
                             static_cast<uint32_t>(format));
            return 0;
    }
}
VkImageMemoryBarrier2
create_barrier(const ImageBarrier& barrier, VulkanResourceManager& manager)
{
    auto& img = manager.get_resource_from_handle(barrier.handle);

    return { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
             .pNext = nullptr,
             .srcStageMask = barrier.srcStageMask,
             .srcAccessMask = barrier.srcAccessMask,
             .dstStageMask = barrier.dstStageMask,
             .dstAccessMask = barrier.dstAccessMask,
             .oldLayout = barrier.oldLayout,
             .newLayout = barrier.newLayout,
             .srcQueueFamilyIndex = barrier.srcQueueFamilyIndex,
             .dstQueueFamilyIndex = barrier.dstQueueFamilyIndex,
             .image = img.image,
             .subresourceRange = Initialisers::image_subresource_range(
                 format_to_image_aspect(img.imageFormat)) };
}

void
ExecutionPlan::add_barriers(std::vector<PassSubmitInfo>& preBarriers,
                            const ExecutePass& pass,
                            VulkanResourceManager& manager)
{
    auto& [bufferBarriers, imageBarriers, _, __] = preBarriers[pass.pass.index];

    bufferBarriers.reserve(pass.bufferBarriers.size() +
                           pass.bufferTransfers.size());
    imageBarriers.reserve(pass.imageBarriers.size() +
                          pass.imageTransfers.size());

    for (const auto& barrier : pass.bufferBarriers) {
        bufferBarriers.push_back(create_barrier(barrier, manager));
    }
    for (const auto& barrier : pass.imageBarriers) {
        imageBarriers.push_back(create_barrier(barrier, manager));
    }
}

void
ExecutionPlan::add_transfers(std::vector<PassSubmitInfo>& preBarriers,
                             std::vector<PassSubmitInfo>& postBarriers,
                             const std::vector<uint32_t>& passToOrder,
                             const ExecutePass& pass,
                             PassServer& passServer,
                             VulkanResourceManager& manager)
{

    auto& pre = preBarriers[pass.pass.index];

    auto helper = [&](auto& barriers, const auto& transfers, auto barrierPtr) {
        for (const auto& transfer : transfers) {
            barriers.push_back(create_barrier(transfer.barrier, manager));

            const auto& semaphore =
                passServer.get_semaphore(transfer.barrier.srcQueueFamilyIndex);
            // Each pass is assigned a signal value, based on its ordering in
            // execution. If another pass relies on the semaphore, it will
            // signal it with this value
            uint64_t signalValue =
                passToOrder[transfer.semaphore.index] + semaphore.current;

            pre.waitSemaphoreInfo.emplace_back(
                VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                nullptr,
                semaphore.semaphore,
                signalValue,
                transfer.barrier.dstStageMask,
                0);

            auto& post = postBarriers[transfer.semaphore.index];

            (post.*barrierPtr)
                .push_back(create_barrier(transfer.barrier, manager));

            if (post.signalSemaphoreInfo.sType !=
                VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO) {
                post.signalSemaphoreInfo = {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                    .pNext = nullptr,
                    .semaphore = semaphore.semaphore,
                    .value = signalValue,
                    .stageMask = 0,
                    .deviceIndex = 0
                };
            }

            // This signal semaphore represents the completion of an entire
            // pass, and accesses to all relevant resources As such we must
            // accumulate the stage masks
            post.signalSemaphoreInfo.stageMask |= transfer.barrier.srcStageMask;
        }
    };

    helper(pre.buffers, pass.bufferTransfers, &PassSubmitInfo::buffers);

    helper(pre.images, pass.imageTransfers, &PassSubmitInfo::images);
}

void
ExecutionPlan::add_merges(
    const ExecutePass& pass,
    std::vector<PassSubmitInfo>& preBarriers,
    const std::unordered_map<Handle<AllocatedBuffer>, Handle<RenderPass>>&
        bufferMergePoints,
    const std::unordered_map<Handle<AllocatedImage>, Handle<RenderPass>>&
        imageMergePoints,
    VulkanResourceManager& manager)
{
    for (const auto& merge : pass.bufferMerges) {
        auto mergePoint = bufferMergePoints.at(merge.handle);

        // Find the buffer
        VkBuffer buffer = manager.get_resource_from_handle(merge.handle).buffer;

        for (auto& barrier : preBarriers[mergePoint.index].buffers) {
            if (barrier.buffer == buffer) {
                barrier.dstAccessMask |= merge.access;
                barrier.dstStageMask |= merge.stages;
                break;
            }
        }
    }

    for (const auto& merge : pass.imageMerges) {
        auto mergePoint = imageMergePoints.at(merge.handle);

        // Find the image
        VkImage image = manager.get_resource_from_handle(merge.handle).image;

        for (auto& barrier : preBarriers[mergePoint.index].images) {
            if (barrier.image == image) {
                barrier.dstAccessMask |= merge.access;
                barrier.dstStageMask |= merge.stages;
                break;
            }
        }
    }
}

void
mark_trackers(
    const RenderPass& pass,
    const std::unordered_map<Cel::Handle<RenderPass>, RenderPass>& passes,
    std::unordered_map<Cel::Handle<AllocatedBuffer>, Cel::Handle<RenderPass>>&
        bufferMergePoints,
    std::unordered_map<Cel::Handle<AllocatedImage>, Cel::Handle<RenderPass>>&
        imageMergePoints)
{
    // We want to find the first read of the current resource data that is on
    // this queue
    auto read_helper = [&](const auto& reads, auto& map) {
        for (const auto& read : reads) {
            if (!map.contains(read.id) ||
                passes.at(map.at(read.id)).queue != pass.queue) {
                map.emplace(read.id, pass.id);
            }
        }
    };

    read_helper(pass.bufferReads, bufferMergePoints);
    read_helper(pass.imageReads, imageMergePoints);

    // Unset any written resources
    auto write_helper = [&](const auto& writes, auto& map) {
        for (const auto& write : writes) {
            map.erase(write.id);
        }
    };

    write_helper(pass.bufferWrites, bufferMergePoints);
    write_helper(pass.imageWrites, imageMergePoints);
}

void
ExecutionPlan::record_barriers(VkCommandBuffer cmd, PassSubmitInfo& info)
{
    VkDependencyInfo dependency{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                 .pNext = nullptr,
                                 .dependencyFlags = 0,
                                 .memoryBarrierCount = 0,
                                 .pMemoryBarriers = nullptr,
                                 .bufferMemoryBarrierCount =
                                     static_cast<uint32_t>(info.buffers.size()),
                                 .pBufferMemoryBarriers = info.buffers.data(),
                                 .imageMemoryBarrierCount =
                                     static_cast<uint32_t>(info.images.size()),
                                 .pImageMemoryBarriers = info.images.data() };

    const VkCommandBufferBeginInfo beginInfo =
        Initialisers::command_buffer_begin_info(
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    vkBeginCommandBuffer(cmd, &beginInfo);
    vkCmdPipelineBarrier2(cmd, &dependency);
    vkEndCommandBuffer(cmd);
}

void
ExecutionPlan::create_submit_infos(
    std::array<std::vector<VkSubmitInfo2>, QUEUE_COUNT>& submits,

    std::vector<ExecutePass>& plan,
    const std::unordered_map<Handle<RenderPass>, RenderPass>& passes,
    PassServer& passServer,
    VulkanResourceManager& manager)
{
    // Perhaps at a later stage it would be preferable to merge two or more
    // submits when possible

    // Lazy way of getting the max number of passes. Assumes of course that
    // we're assigning pass handles statically at startup.
    static auto [maxPasses] =
        Passes::HandleAllocator::allocate_pass("greatest_handle");

    // upper limit
    constexpr VkCommandBufferSubmitInfo defaultCmdSubmit = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = nullptr,
        .deviceMask = 0
    };
    std::vector recorded{ maxPasses * 3, defaultCmdSubmit };

    // Barriers inserted before & after the pass
    std::vector<PassSubmitInfo> preBarriers{ maxPasses };
    std::vector<PassSubmitInfo> postBarriers{ maxPasses };

    // A tracker is kept marking the last pass to read a resource
    // This info is used to determine what we're merging to
    std::unordered_map<Handle<AllocatedBuffer>, Handle<RenderPass>>
        bufferMergePoints;
    std::unordered_map<Handle<AllocatedImage>, Handle<RenderPass>>
        imageMergePoints;

    // A map of pass.id to it's place in the total execution order
    // Used to grant a stable value for timeline semaphores
    std::vector<uint32_t> passToOrder{ maxPasses, 0 };

    // Just the ordering of the passes and their queues, so I don't have to load
    // the original execution back into memory for recording
    std::vector<std::pair<Handle<RenderPass>, uint32_t>> ordering{
        plan.size()
    };

    for (const auto& [i, pass] : std::views::enumerate(plan)) {
        const auto& renderPass = passes.at(pass.pass);

        mark_trackers(renderPass, passes, bufferMergePoints, imageMergePoints);

        passToOrder[pass.pass.index] = i;
        ordering[i] = { pass.pass, passes.at(pass.pass).queue };

        add_barriers(preBarriers, pass, manager);
        add_transfers(
            preBarriers, postBarriers, passToOrder, pass, passServer, manager);
        add_merges(
            pass, preBarriers, bufferMergePoints, imageMergePoints, manager);
    }

    for (auto [pass, queue] : ordering) {

        auto& submit = submits[passServer.queueToIndex[queue]].emplace_back(
            VK_STRUCTURE_TYPE_SUBMIT_INFO_2, nullptr, 0);

        auto& pre = preBarriers[pass.index];
        auto& post = postBarriers[pass.index];

        recorded[pass.index * 3 + 1].commandBuffer =
            passServer.passCmdBuffers[passServer.currentFrame].at(pass).first;

        vkEndCommandBuffer(recorded[pass.index * 3 + 1].commandBuffer);

        uint32_t offset = 1;
        uint32_t count = 1;

        if (!pre.buffers.empty() || !pre.images.empty()) {
            offset = 0;
            count++;

            // Record pre barriers
            const auto cmd = passServer.get_prepost_cmd_buffer(queue);
            recorded[pass.index * 3].commandBuffer = cmd;
            record_barriers(cmd, pre);
        }
        if (!post.buffers.empty() || !post.images.empty()) {
            count++;

            // Record post barriers
            const auto cmd = passServer.get_prepost_cmd_buffer(queue);
            recorded[pass.index * 3 + 2].commandBuffer = cmd;
            record_barriers(cmd, post);
        }

        submit.commandBufferInfoCount = count;
        submit.pCommandBufferInfos = recorded.data() + pass.index * 3 + offset;

        submit.signalSemaphoreInfoCount = 1;
        submit.pSignalSemaphoreInfos = &post.signalSemaphoreInfo;

        submit.waitSemaphoreInfoCount = pre.waitSemaphoreInfo.size();
        submit.pWaitSemaphoreInfos = pre.waitSemaphoreInfo.data();
    }

    // Tick cpu semaphore data
    for (auto& semaphore : passServer.semaphores) {
        semaphore.current += maxPasses;
    }
}

void
ExecutionPlan::execute(
    std::vector<ExecutePass>& plan,
    const std::unordered_map<Handle<RenderPass>, RenderPass>& passes,
    const Handle<RenderPass> presentPass,
    PassServer& passServer,
    Swapchain& swapchain,
    VulkanResourceManager& manager)
{

    std::array<std::vector<VkSubmitInfo2>, QUEUE_COUNT> submits;

    create_submit_infos(submits, plan, passes, passServer, manager);

    uint32_t swapchainIndex;
    VkResult result = vkAcquireNextImageKHR(
        passServer.device,
        swapchain.swapchain,
        UINT64_MAX,
        passServer.acquireSemaphores[passServer.currentFrame],
        VK_NULL_HANDLE,
        &swapchainIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Introduce code to handle resizing swapchain
        fmt::println("Out of date swapchain, needs resizing");
        return;
    }

    {
        // Copy image to swapchain on the present pass
        const auto cmd = passServer.get_cmd_buffer(presentPass);

        const auto& drawImage = passServer.get_resource(Passes::drawImage);

        Utils::copy_image_to_image(cmd,
                                   drawImage.image,
                                   swapchain.images[swapchainIndex],
                                   passServer.extent,
                                   swapchain.extent);

        Utils::transition_image_layout(cmd,
                                       swapchain.images[swapchainIndex],
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    }

    VkSemaphoreSubmitInfo swapSemInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = swapchain.submitSemaphores[swapchainIndex],
        .value = 0,
        .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .deviceIndex = 0
    };

    const auto presentQueue = passes.at(presentPass).queue;

    for (size_t i = 0; i < QUEUE_COUNT; i++) {

        if (submits[i].empty()) {
            continue;
        }

        // If this is the queue we're presenting from, signal the present
        // semaphore and fence once it's done
        VkFence fence = nullptr;
        if (i == passServer.queueToIndex[presentQueue]) {
            fence = passServer.fences[passServer.currentFrame];

            auto& final = submits[i].back();

            // The very final pass shouldn't signal any semaphores currently, so
            // we don't need a list.
            assert(final.signalSemaphoreInfoCount == 1);
            final.signalSemaphoreInfoCount = 1;
            final.pSignalSemaphoreInfos = &swapSemInfo;
        }

        vk_check(vkQueueSubmit2(passServer.queues[i].queue,
                                submits[i].size(),
                                submits[i].data(),
                                fence));
    }

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain.submitSemaphores[swapchainIndex],
        .swapchainCount = 1,
        .pSwapchains = &swapchain.swapchain,
        .pImageIndices = &swapchainIndex,
        .pResults = nullptr
    };

    vkQueuePresentKHR(
        passServer.queues[passServer.queueToIndex[presentQueue]].queue,
        &presentInfo);
}

void
ExecutionPlan::add_execution_to_list(ExecutionPlan* plan,
                                     std::vector<ExecutePass>& list)
{
    if (plan->original != nullptr) {
        add_execution_to_list(plan->original, list);
    }
    list.insert(list.end(), plan->execution.begin(), plan->execution.end());
}
