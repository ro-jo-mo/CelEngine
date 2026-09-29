#include "renderer/render-graph/ExecutionPlan.h"

#include "common/Graph.h"
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
        // Depths
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT;

            // Colours
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
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

    return { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
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
                             SubmitSplitter& splitter,
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

            splitter.add_acquire(pass.pass,
                                 passToOrder[pass.pass.index],
                                 transfer.barrier.srcQueueFamilyIndex,
                                 transfer.barrier.dstQueueFamilyIndex);
            splitter.add_release(transfer.signalPass,
                                 passToOrder[transfer.signalPass.index],
                                 transfer.barrier.srcQueueFamilyIndex,
                                 transfer.barrier.dstQueueFamilyIndex);

            // Each pass is assigned a signal value, based on its ordering in
            // execution. If another pass relies on the semaphore, it will
            // signal it with this value
            uint64_t signalValue =
                passToOrder[transfer.signalPass.index] + semaphore.current;

            pre.waitSemaphoreInfo.emplace_back(
                VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                nullptr,
                semaphore.semaphore,
                signalValue,
                transfer.barrier.dstStageMask,
                0);

            auto& post = postBarriers[transfer.signalPass.index];

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
            // pass, and accesses to all relevant resources
            // As such we must accumulate the stage masks
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

ExecutionPlan::SubmitData
ExecutionPlan::create_submit_infos(
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

    constexpr VkCommandBufferSubmitInfo defaultCmdSubmit = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext = nullptr,
        .commandBuffer = nullptr,
        .deviceMask = 0
    };

    SubmitData data{ .preBarriers = std::vector<PassSubmitInfo>(maxPasses),
                     .postBarriers = std::vector<PassSubmitInfo>(maxPasses),
                     .recorded = std::vector(maxPasses * 3, defaultCmdSubmit) };

    auto& [submits, splits, preBarriers, postBarriers, recorded] = data;
    SubmitSplitter splitter{};

    // A tracker is kept marking the last pass to read a resource
    // This info is used to determine what we're merging to
    std::unordered_map<Handle<AllocatedBuffer>, Handle<RenderPass>>
        bufferMergePoints;
    std::unordered_map<Handle<AllocatedImage>, Handle<RenderPass>>
        imageMergePoints;

    // A map of pass.id to it's place in the total execution order
    // Used to grant a stable value for timeline semaphores
    // Base pass signals 1, rest signal > 1
    std::vector<uint32_t> passToOrder(maxPasses, 1);

    // Just the ordering of the passes and their queues, so I don't have to load
    // the original execution back into memory for recording
    std::vector<std::pair<Handle<RenderPass>, uint32_t>> ordering{
        plan.size()
    };

    for (const auto& [i, pass] : std::views::enumerate(plan)) {
        const auto& renderPass = passes.at(pass.pass);

        mark_trackers(renderPass, passes, bufferMergePoints, imageMergePoints);

        passToOrder[pass.pass.index] += i;
        ordering[i] = { pass.pass, passes.at(pass.pass).queue };

        add_barriers(preBarriers, pass, manager);
        add_transfers(preBarriers,
                      postBarriers,
                      passToOrder,
                      splitter,
                      pass,
                      passServer,
                      manager);
        add_merges(
            pass, preBarriers, bufferMergePoints, imageMergePoints, manager);
    }

    // TODO: SET BASE PASS SUBMITS ON ALL QUEUES

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

            for (const auto& res : pre.buffers) {
                assert(res.dstQueueFamilyIndex == queue ||
                       res.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
            }
            for (const auto& res : pre.images) {
                assert(res.dstQueueFamilyIndex == queue ||
                       res.dstQueueFamilyIndex == VK_QUEUE_FAMILY_IGNORED);
            }

            record_barriers(cmd, pre);
        }
        if (!post.buffers.empty() || !post.images.empty()) {
            count++;

            // Record post barriers
            const auto cmd = passServer.get_prepost_cmd_buffer(queue);
            recorded[pass.index * 3 + 2].commandBuffer = cmd;

            for (const auto& res : post.buffers) {
                assert(res.srcQueueFamilyIndex == queue);
            }
            for (const auto& res : post.images) {
                assert(res.srcQueueFamilyIndex == queue);
            }

            record_barriers(cmd, post);
        }

        submit.commandBufferInfoCount = count;
        submit.pCommandBufferInfos = recorded.data() + pass.index * 3 + offset;

        // Only set if needed
        if (post.signalSemaphoreInfo.sType ==
            VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO) {
            submit.signalSemaphoreInfoCount = 1;
            submit.pSignalSemaphoreInfos = &post.signalSemaphoreInfo;
        } else {
            submit.signalSemaphoreInfoCount = 0;
        }

        submit.waitSemaphoreInfoCount = pre.waitSemaphoreInfo.size();
        submit.pWaitSemaphoreInfos = pre.waitSemaphoreInfo.data();
    }

    // Tick cpu semaphore data
    for (auto& semaphore : passServer.semaphores) {
        semaphore.current += maxPasses;
    }

    data.splits = splitter.compile(passServer.queues);

    return data;
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
    // Get swapchain image
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

    // Record commands for the present pass
    {
        // Copy image to swapchain on the present pass
        const auto cmd = passServer.get_cmd_buffer(presentPass);

        const auto& drawImage = passServer.get_resource(Passes::drawImage);

        Utils::transition_image_layout(cmd,
                                       swapchain.images[swapchainIndex],
                                       VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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

    // Create submit infos
    // The pres / posts / recorded are mainly here to keep the data alive until
    // submission
    auto [submits, splits, pres, posts, recorded] =
        create_submit_infos(plan, passes, passServer, manager);

    // The last submission should be presentation, which should wait on the
    // acquire semaphore and signal the present semaphore swapchain semaphore

    VkSemaphoreSubmitInfo acquireSemInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = passServer.acquireSemaphores[passServer.currentFrame],
        .value = 0,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .deviceIndex = 0
    };
    VkSemaphoreSubmitInfo presentSemInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .semaphore = swapchain.presentSemaphores[swapchainIndex],
        .value = 0,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .deviceIndex = 0
    };

    const auto presentQueue = passes.at(presentPass).queue;

    {
        auto& present = submits[passServer.queueToIndex[presentQueue]].back();
        present.signalSemaphoreInfoCount = 1;
        present.pSignalSemaphoreInfos = &presentSemInfo;

        auto& pre = pres[presentPass.index];

        pre.waitSemaphoreInfo.push_back(acquireSemInfo);

        present.waitSemaphoreInfoCount = pre.waitSemaphoreInfo.size();
        present.pWaitSemaphoreInfos = pre.waitSemaphoreInfo.data();
    }

    // Finally we can actually submit our data

    for (auto& split : splits) {

        const auto index = passServer.queueToIndex[split.queue];

        // UINT32_MAX is used as a flag for the last submit for this queue
        // We must fill the in the info manually
        if (split.end == UINT32_MAX) {

            // Skip if we already finished this
            if (split.start == submits[index].size()) {
                continue;
            }

            split.end = submits[index].size();
        }

        VkFence fence = nullptr;
        if (split.queue == presentQueue) {
            fence = passServer.fences[passServer.currentFrame];
        }

        vk_check(vkQueueSubmit2(passServer.queues[index].queue,
                                split.end - split.start,
                                submits[index].data() + split.start,
                                fence));
    }

    // Lastly present
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &swapchain.presentSemaphores[swapchainIndex],
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

void
ExecutionPlan::SubmitSplitter::add_release(Handle<RenderPass> pass,
                                           const uint32_t passOrder,
                                           uint32_t releaseQueue,
                                           uint32_t acquireQueue)
{
    releaseAndAcquires[passOrder].emplace_back(
        pass, false, releaseQueue, acquireQueue);
}

void
ExecutionPlan::SubmitSplitter::add_acquire(Handle<RenderPass> pass,
                                           const uint32_t passOrder,
                                           uint32_t releaseQueue,
                                           uint32_t acquireQueue)
{
    releaseAndAcquires[passOrder].emplace_back(
        pass, true, releaseQueue, acquireQueue);
}

std::vector<ExecutionPlan::SubmitSplitter::SubmitSplit>
ExecutionPlan::SubmitSplitter::compile(
    const std::array<Queue, QUEUE_COUNT>& queues)
{
    // if (releasingTo[i][j])
    // if queue i is releasing a resource to queue j in this submit
    std::array<std::array<bool, 16>, 16> releasingTo{};

    // We're currently up to submit: "counters[queue]"
    std::array<uint32_t, 16> counters{};
    // In progress split for each queue
    std::array<SubmitSplit, 16> splits{};

    for (const auto& [i, split] : std::views::enumerate(splits)) {
        split.queue = i;
    }

    std::vector<SubmitSplit> out;

    for (const auto& transfers : releaseAndAcquires | std::views::values) {
        for (const auto& transfer : transfers) {
            if (transfer.isAcquire) {

                // If the acquiring queue is currently has a release to the
                // releasing queue We must split on the acquiring queue
                if (releasingTo[transfer.acquireQueue][transfer.releaseQueue]) {
                    splits[transfer.acquireQueue].end =
                        counters[transfer.acquireQueue];

                    out.emplace_back(splits[transfer.acquireQueue]);

                    // Reset for the next split
                    splits[transfer.acquireQueue].start =
                        counters[transfer.acquireQueue];
                    releasingTo[transfer.acquireQueue] = {};
                }

                counters[transfer.acquireQueue]++;

            } else {
                counters[transfer.releaseQueue]++;
                releasingTo[transfer.releaseQueue][transfer.acquireQueue] =
                    true;
            }
        }
    }

    // If queue 0 starts by acquiring from queue 1, 2

    Common::Graph<uint32_t> finalSubmitGraph{};

    for (const auto& queue : queues) {
        // if currently releasing to another queue, then we must submit before
        // them
        finalSubmitGraph.add_node(queue.family);

        for (const auto [releasingFamily, isReleasing] :
             std::views::enumerate(releasingTo[queue.family])) {
            if (isReleasing) {
                finalSubmitGraph.add_edge(queue.family, releasingFamily);
            }
        }
    }

    const auto finalOrder = finalSubmitGraph.simple_ordering();

    for (const auto queue : finalOrder) {

        auto& split = splits[queue];
        split.end = UINT32_MAX;

        out.emplace_back(splits[queue]);
    }

    return out;
}
