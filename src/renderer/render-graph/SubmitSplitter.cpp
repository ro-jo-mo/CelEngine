#include "renderer/render-graph/SubmitSplitter.h"

#include <algorithm>
#include <fmt/base.h>
#include <ranges>

Cel::Renderer::RenderGraph::SubmitSplitter::SubmitSplitter(
    const uint32_t maxPasses,
    const std::array<uint32_t, 16>& queueToIndex,
    const std::vector<uint32_t>& passToOrder,
    const std::vector<uint32_t>& passToQueueOrder)
    : queueToIndex(queueToIndex)
    , passToOrder(passToOrder)
    , passToQueueOrder(passToQueueOrder)
{
    for (auto& list : passes) {
        list = std::vector<std::optional<Pass>>(maxPasses);
    }
}

void
Cel::Renderer::RenderGraph::SubmitSplitter::add_release_and_acquire(
    const Handle<RenderPass> releasing,
    const Handle<RenderPass> acquiring,
    const uint32_t releaseQueue,
    const uint32_t acquireQueue)
{

    auto& acquire =
        passes[queueToIndex[acquireQueue]][passToQueueOrder[acquiring.index]];

    if (!acquire) {
        acquire = Pass{};
    }

    // We depend on the latest pass in the queue we're acquiring from
    auto& dependsOn = acquire.value().dependsOn[queueToIndex[releaseQueue]];

    if (dependsOn == UINT32_MAX) {
        dependsOn = passToQueueOrder[releasing.index];
    } else {
        dependsOn = std::max(passToQueueOrder[releasing.index], dependsOn);
    }
}

bool
Cel::Renderer::RenderGraph::SubmitSplitter::branch_queue(
    const uint32_t targetSubmit,
    const uint32_t currentQueue,
    std::array<uint32_t, QUEUE_COUNT>& _upTo,
    std::array<std::array<bool, QUEUE_COUNT>, QUEUE_COUNT>& _isAcquiringFrom,
    std::vector<SubmitSplit>& out,
    const std::array<size_t, QUEUE_COUNT>& submitSizes)
{

    auto& submits = passes[currentQueue];

    // Since we'll need to backtrack on occasion, we need to store the original
    // data
    const auto originalOutSize = out.size();
    auto upTo = _upTo;
    auto isAcquiringFrom = _isAcquiringFrom;

    for (uint32_t i = upTo[currentQueue]; i < submits.size(); i++) {

        if (!submits[i]) {
            continue;
        }

        auto& submit = submits[i].value();

        for (const auto [dependencyQueue, dependency] :
             std::views::enumerate(submit.dependsOn)) {

            // Has it been set?
            if (dependency == UINT32_MAX) {
                continue;
            }

            // Have we already met the dependency?
            if (upTo[dependencyQueue] > dependency) {

                continue;
            }

            isAcquiringFrom[currentQueue][dependencyQueue] = true;

            // If this dependency is already dependent on us, or if after
            // searching the branch we can't solve it, this split is finished
            if (isAcquiringFrom[dependencyQueue][currentQueue] ||
                !branch_queue(dependency,
                              dependencyQueue,
                              upTo,
                              isAcquiringFrom,
                              out,
                              submitSizes)) {

                // If we've met our target, we can submit this. Otherwise,
                // discard
                if (i >= targetSubmit) {

                    out.push_back({ .start = upTo[currentQueue],
                                    .end = i,
                                    .queue = currentQueue });

                    upTo[currentQueue] = i;

                    // After finalising a submit on this queue, we're no longer
                    // acquiring from anywhere
                    isAcquiringFrom[currentQueue] = {};

                    // Commit changes
                    _isAcquiringFrom = isAcquiringFrom;
                    _upTo = upTo;

                    return true;
                }

                // Discard changes if not committing
                out.resize(originalOutSize);

                return false;
            }
        }
    }

    // In the case we arrive at the end, add split to the end of this queue
    out.push_back({ .start = upTo[currentQueue],
                    .end = UINT32_MAX,
                    .queue = currentQueue });

    // Mark the queue as finished
    upTo[currentQueue] = UINT32_MAX;

    // Commit changes
    _isAcquiringFrom = isAcquiringFrom;
    _upTo = upTo;

    return true;
}

std::vector<Cel::Renderer::RenderGraph::SubmitSplitter::SubmitSplit>
Cel::Renderer::RenderGraph::SubmitSplitter::compile(
    const std::array<size_t, QUEUE_COUNT>& submitSizes)
{
    std::vector<SubmitSplit> out;

    // Which submit are we up to in this queue?
    std::array<uint32_t, QUEUE_COUNT> upTo{};

    // When we don't have a dedicated queue transfer / compute, we fall back to
    // graphics. In this case there will be 0 submits

    for (uint32_t i = 0; i < QUEUE_COUNT; i++) {
        if (submitSizes[i] == 0) {
            upTo[i] = UINT32_MAX;
        }
    }

    // Is queue i acquiring from queue j this split? isAcquiringFrom[i][j]
    std::array<std::array<bool, QUEUE_COUNT>, QUEUE_COUNT> isAcquiringFrom{};

    // Iter each queues submission

    uint32_t counter = 0;
    while (true) {

        if (std::ranges::all_of(upTo,
                                [](auto val) { return val == UINT32_MAX; })) {
            break;
        }

        if (upTo[counter] == UINT32_MAX) {
            continue;
        }

        branch_queue(0, counter, upTo, isAcquiringFrom, out, submitSizes);

        counter++;
        counter %= QUEUE_COUNT;
    }

    return out;
}
