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
create_barrier(const BufferBarrier& barrier, const PassServer& server)
{
    return { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
             .pNext = nullptr,
             .srcStageMask = barrier.srcStageMask,
             .srcAccessMask = barrier.srcAccessMask,
             .dstStageMask = barrier.dstStageMask,
             .dstAccessMask = barrier.dstAccessMask,
             .srcQueueFamilyIndex = barrier.srcQueueFamilyIndex,
             .dstQueueFamilyIndex = barrier.dstQueueFamilyIndex,
             .buffer = server.get_resource(barrier.handle).buffer,
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
create_barrier(const ImageBarrier& barrier, const PassServer& server)
{

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
             .image = server.get_resource(barrier.handle).image,
             .subresourceRange =
                 Initialisers::image_subresource_range(format_to_image_aspect(
                     server.get_resource(barrier.handle).imageFormat)) };
}

// Unfortunately to reduce the number of passes we do over the data,
void
ExecutionPlan::add_barriers(
    std::unordered_map<Handle<RenderPass>, BarrierSet>& preBarriers,
    const ExecutePass& pass,
    const PassServer& passServer)
{
    auto& [bufferBarriers, imageBarriers] = preBarriers[pass.pass];

    bufferBarriers.reserve(pass.bufferBarriers.size() +
                           pass.bufferTransfers.size());
    imageBarriers.reserve(pass.imageBarriers.size() +
                          pass.imageTransfers.size());

    for (const auto& barrier : pass.bufferBarriers) {
        bufferBarriers.push_back(create_barrier(barrier, passServer));
    }
    for (const auto& barrier : pass.imageBarriers) {
        imageBarriers.push_back(create_barrier(barrier, passServer));
    }
}

void
ExecutionPlan::add_transfers(
    std::vector<VkSubmitInfo2> submits,
    std::unordered_map<Handle<RenderPass>, BarrierSet>& preBarriers,
    std::unordered_map<Handle<RenderPass>, BarrierSet>& postBarriers,
    const ExecutePass& pass,
    PassServer& passServer)
{

    auto& [preBuffers, preImages] = preBarriers[pass.pass];

    for (const auto& transfer : pass.bufferTransfers) {
        preBuffers.push_back(create_barrier(transfer.barrier, passServer));

        // Request semaphore ??????? Should I take the submit info ?
        // Probably take the submit info, if semaphore = nullptr, create
        // semaphore
        auto& [postBuffers, _] = preBarriers[transfer.semaphore];
        postBuffers.push_back(create_barrier(transfer.barrier, passServer));

        auto& priorSubmit = submits[transfer.semaphore.index];
        if (priorSubmit.pSignalSemaphoreInfos == nullptr) {
            // Assign semaphore
            VkSemaphoreCreateInfo x{ .} vkCreateSemaphore()
        }
    }
    for (const auto& transfer : pass.imageTransfers) {
        preImages.push_back(create_barrier(transfer.barrier, passServer));
    }
}

void
ExecutionPlan::execute(std::vector<ExecutePass>& plan, PassServer& passServer)
{
    // Perhaps at a later stage it would be preferable to merge two or more
    // submits when possible

    // Lazy way of getting the max number of passes. Assumes of course that
    // we're assigning pass handles statically at startup.
    static auto [maxPasses] =
        Passes::HandleAllocator::allocate_pass("greatest_handle");
    constexpr VkSubmitInfo2 defaultSubmit{ .sType =
                                               VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
                                           .pNext = nullptr,
                                           .flags = 0,
                                           .waitSemaphoreInfoCount = 0,
                                           .pWaitSemaphoreInfos = nullptr,
                                           .commandBufferInfoCount = 0,
                                           .pCommandBufferInfos = nullptr,
                                           .signalSemaphoreInfoCount = 0,
                                           .pSignalSemaphoreInfos = nullptr };

    std::vector submits{ maxPasses, defaultSubmit };

    // upper limit
    std::vector<VkCommandBufferSubmitInfo> recorded{ maxPasses * 3 };

    // Each pass has a single submit associated with it.
    // Data for its submit is placed consecutively in these arrays
    std::vector<VkSemaphoreSubmitInfo> waitSemaphoreInfos;
    std::vector<VkSemaphoreSubmitInfo> signalSemaphoreInfos;
    std::vector<VkCommandBufferSubmitInfo> commandBufferInfos;

    // A tracker is kept marking the last pass to read a resource
    // This info is used to determine what we're merging to
    std::unordered_map<Handle<AllocatedBuffer>, Handle<RenderPass>>
        lastPassToAccessBuffer;
    std::unordered_map<Handle<AllocatedImage>, Handle<RenderPass>>
        lastPassToAccessImage;

    for (const auto& pass : plan) {

        auto& submit = submits[pass.pass.index];

        uint32_t totalCmdsBuffs = 1;

        // Add barriers
        if (!pass.bufferBarriers.empty() || !pass.imageBarriers.empty()) {
            totalCmdsBuffs++;

            add_barriers(recorded, pass, passServer);
        }

        VkCommandBufferSubmitInfo
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .pNext =,
            .commandBuffer =, .deviceMask =
        }
        // Add transfers
        // Current question, do we merge to buffer transfers at all? Presumably
        // yes.
        if (!pass.bufferTransfers.empty() || !pass.imageTransfers.empty()) {
        }
        // Add  merges
    }
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
