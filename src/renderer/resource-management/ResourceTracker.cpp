#include "renderer/resource-management/ResourceTracker.h"

#include "renderer/passes/Passes.h"

Cel::Renderer::BranchingResourceTracker::BranchingResourceTracker(
    const VulkanResourceManager& manager)
    : VulkanResourceManager(manager)
{
    device = nullptr;
    allocator = nullptr;
}

void
Cel::Renderer::BranchingResourceTracker::manifest_resource(
    Handle<AllocatedBuffer> handle)
{
    const auto alias = alias_resource(bufferPool.requirements.at(handle));

    if (alias != UINT32_MAX) {
        bufferPool.states.emplace(handle, std::get<2>(bufferPool.freed[alias]));

        changeLog.emplace_back(ManifestBuffer{
            .handle = handle, .aliased = bufferPool.freed[alias] });

        bufferPool.freed.erase(bufferPool.freed.begin() + alias);
    } else {
        bufferPool.states.emplace(handle, BufferAccess{});

        changeLog.emplace_back(ManifestBuffer{ .handle = handle });
    }
}

void
Cel::Renderer::BranchingResourceTracker::manifest_resource(
    Handle<AllocatedImage> handle)
{
    const auto alias = alias_resource(imagePool.requirements.at(handle));

    if (alias != UINT32_MAX) {
        imagePool.states.emplace(handle, std::get<2>(imagePool.freed[alias]));

        changeLog.emplace_back(ManifestImage{
            .handle = handle, .aliased = imagePool.freed[alias] });

        imagePool.freed.erase(imagePool.freed.begin() + alias);
    } else {
        imagePool.states.emplace(handle, ImageAccess{});

        changeLog.emplace_back(ManifestImage{ .handle = handle });
    }
}

bool
Cel::Renderer::BranchingResourceTracker::is_dirty(
    Handle<AllocatedBuffer> handle) const
{
    return bufferPool.dirty.contains(handle);
}

bool
Cel::Renderer::BranchingResourceTracker::is_dirty(
    Handle<AllocatedImage> handle) const
{
    return imagePool.dirty.contains(handle);
}

void
Cel::Renderer::BranchingResourceTracker::mark_dirty(
    const Handle<AllocatedBuffer> handle)
{
    changeLog.emplace_back(MarkBuffer{
        .handle = handle, .marking = bufferPool.dirty.contains(handle) });

    bufferPool.mark_dirty(handle);
}

void
Cel::Renderer::BranchingResourceTracker::mark_dirty(
    const Handle<AllocatedImage> handle)
{
    changeLog.emplace_back(MarkImage{
        .handle = handle, .marking = imagePool.dirty.contains(handle) });

    imagePool.mark_dirty(handle);
}

void
Cel::Renderer::BranchingResourceTracker::mark_clean(
    const Handle<AllocatedBuffer> handle)
{
    changeLog.emplace_back(MarkBuffer{
        .handle = handle, .marking = bufferPool.dirty.contains(handle) });

    bufferPool.mark_clean(handle);
}

void
Cel::Renderer::BranchingResourceTracker::mark_clean(
    const Handle<AllocatedImage> handle)
{
    changeLog.emplace_back(MarkImage{
        .handle = handle, .marking = imagePool.dirty.contains(handle) });

    imagePool.mark_clean(handle);
}

const Cel::Renderer::BufferAccess&
Cel::Renderer::BranchingResourceTracker::get_state(
    Handle<AllocatedBuffer> handle) const
{
    return bufferPool.states.at(handle);
}

const Cel::Renderer::ImageAccess&
Cel::Renderer::BranchingResourceTracker::get_state(
    Handle<AllocatedImage> handle) const
{
    return imagePool.states.at(handle);
}

void
Cel::Renderer::BranchingResourceTracker::set_state(
    Handle<AllocatedBuffer> handle,
    const BufferAccess& state)
{
    changeLog.emplace_back(SetBufferState{
        .handle = handle, .state = bufferPool.states.at(handle) });

    bufferPool.states.insert_or_assign(handle, state);
}

void
Cel::Renderer::BranchingResourceTracker::set_state(
    Handle<AllocatedImage> handle,
    const ImageAccess& state)
{
    changeLog.emplace_back(SetImageState{
        .handle = handle, .state = imagePool.states.at(handle) });

    imagePool.states.insert_or_assign(handle, state);
}

void
Cel::Renderer::BranchingResourceTracker::add_checkpoint(
    Handle<RenderGraph::RenderPass> handle)
{
    changeLog.emplace_back(Checkpoint{ .handle = handle });
}

Cel::Handle<Cel::Renderer::RenderGraph::RenderPass>
Cel::Renderer::BranchingResourceTracker::get_last_access(
    Handle<AllocatedBuffer> handle) const
{
    if (lastPassToAccessBuffer.contains(handle)) {
        return lastPassToAccessBuffer.at(handle);
    }

    return Passes::basePass;
}

Cel::Handle<Cel::Renderer::RenderGraph::RenderPass>
Cel::Renderer::BranchingResourceTracker::get_last_access(
    Handle<AllocatedImage> handle) const
{
    if (lastPassToAccessImage.contains(handle)) {
        return lastPassToAccessImage.at(handle);
    }

    return Passes::basePass;
}

void
Cel::Renderer::BranchingResourceTracker::set_last_access(
    Handle<AllocatedBuffer> handle,
    Handle<RenderGraph::RenderPass> pass)
{
    changeLog.emplace_back(
        LastBufferAccess{ .handle = handle, .pass = get_last_access(handle) });

    lastPassToAccessBuffer.insert_or_assign(handle, pass);
}

void
Cel::Renderer::BranchingResourceTracker::set_last_access(
    Handle<AllocatedImage> handle,
    Handle<RenderGraph::RenderPass> pass)
{
    changeLog.emplace_back(
        LastImageAccess{ .handle = handle, .pass = get_last_access(handle) });

    lastPassToAccessImage.insert_or_assign(handle, pass);
}

void
Cel::Renderer::BranchingResourceTracker::rewind()
{
    uint32_t upTo = changeLog.size();

    for (const auto& change : std::views::reverse(changeLog)) {

        const bool finished = std::visit(
            Overload{
                [this](const ManifestBuffer& cmd) {
                    if (cmd.aliased.has_value()) {
                        bufferPool.freed.emplace_back(cmd.aliased.value());
                    }

                    bufferPool.states.erase(cmd.handle);

                    return false;
                },
                [this](const ManifestImage& cmd) {
                    if (cmd.aliased.has_value()) {
                        imagePool.freed.emplace_back(cmd.aliased.value());
                    }

                    imagePool.states.erase(cmd.handle);

                    return false;
                },
                [this](const MarkBuffer& cmd) {
                    if (cmd.marking) {
                        bufferPool.dirty.emplace(cmd.handle);
                    } else {
                        bufferPool.dirty.erase(cmd.handle);
                    }

                    return false;
                },
                [this](const MarkImage& cmd) {
                    if (cmd.marking) {
                        imagePool.dirty.emplace(cmd.handle);
                    } else {
                        imagePool.dirty.erase(cmd.handle);
                    }

                    return false;
                },
                [this](const SetBufferState& cmd) {
                    bufferPool.states.insert_or_assign(cmd.handle, cmd.state);

                    return false;
                },
                [this](const SetImageState& cmd) {
                    imagePool.states.insert_or_assign(cmd.handle, cmd.state);

                    return false;
                },
                [this](const LastBufferAccess& cmd) {
                    lastPassToAccessBuffer.insert_or_assign(cmd.handle,
                                                            cmd.pass);

                    return false;
                },
                [this](const LastImageAccess& cmd) {
                    lastPassToAccessImage.insert_or_assign(cmd.handle,
                                                           cmd.pass);

                    return false;
                },

                [](const Checkpoint&) { return true; } },
            change);

        upTo--;

        if (finished) {
            break;
        }
    }

    changeLog.resize(upTo);
}
