#include "renderer/render-graph/SubmitSplitter.h"

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

void
Cel::Renderer::RenderGraph::SubmitSplitter::branch_queue(
    uint32_t targetSubmit,
    uint32_t currentQueue,
    std::array<uint32_t, QUEUE_COUNT> upTo,
    std::array<std::array<bool, QUEUE_COUNT>, QUEUE_COUNT> isAcquiringFrom,
    std::vector<SubmitSplit>& out)
{

    auto& submits = passes[currentQueue];

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
            if (upTo[dependencyQueue] >= dependency) {
                continue;
            }

            // Otherwise, search through that queue execution, and see how far
            // we can reach
            // Then, check upTo again

            // If we have a dependency, they must run first. Switch to
            // searching this branch, mark a flag that we're doing dis
            // so the other queue can't flip back

            // What we do with dis???
            if (isAcquiringFrom[dependencyQueue][currentQueue]) {
            }

            isAcquiringFrom[currentQueue][dependencyQueue] = true;

            branch_queue(
                dependency, dependencyQueue, upTo, isAcquiringFrom, out);

            // After exploration, has the dependency been fulfilled
            // If not, then this is the end of this submission
            // Have we met our target?

            if (upTo[dependencyQueue] < dependency) {

                // If we've met our target, we can submit this. Otherwise,
                // discard

                // NOTE WE COULD MEET REQUIREMENTS, AND THEN END UP NOT USING aSGPI(UAHGOIU
                if (i >= targetSubmit) {
                    out.push_back({ .start = upTo[currentQueue],
                                    .end = i,
                                    .queue = currentQueue });

                    upTo[currentQueue] = i;
                }

                return;
            }
        }
    }
}

std::vector<Cel::Renderer::RenderGraph::SubmitSplitter::SubmitSplit>
Cel::Renderer::RenderGraph::SubmitSplitter::compile(
    const std::array<size_t, QUEUE_COUNT>& submitSizes)
{
    std::vector<SubmitSplit> out;

    // Which submit are we up to in this queue?
    std::array<uint32_t, QUEUE_COUNT> upTo{};

    // Is queue i acquiring from queue j this split? isAcquiringFrom[i][j]
    std::array<std::array<bool, QUEUE_COUNT>, QUEUE_COUNT> isAcquiringFrom{};

    // Iter each queues submission

    size_t counter = 0;
    while (true) {
        auto& submits = passes[counter];

        counter++;
        counter %= QUEUE_COUNT;
    }

    return out;
}
