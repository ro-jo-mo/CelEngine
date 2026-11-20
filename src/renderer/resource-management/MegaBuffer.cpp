#include "renderer/resource-management/MegaBuffer.h"

#include "../../../include/renderer/vulkan/VulkanUtils.h"
#include "core/Error.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <fmt/base.h>

Cel::Renderer::MegaBuffer::MegaBuffer(const BufferRequirements& req,
                                      const std::string& name,
                                      VulkanResourceManager& manager,
                                      const uint32_t alignment)
    : handle(manager.get_handle_from_requirements(req, name))
    , buffer(manager.get_resource_from_handle(handle))
    , alignment(alignment)
    , totalSize(req.allocSize)
{
}

uint32_t
Cel::Renderer::MegaBuffer::allocate(const void* data, const uint32_t size)
{
    if (uploadOffset == UINT32_MAX) {
        uploadOffset = currentUsage;
    }

    auto newSize = size;
    if (alignment != 0 && size % alignment != 0) {
        newSize += alignment - (size % alignment);
    }

    // Store in buffer until upload
    const uint32_t start = dataToUpload.size();
    dataToUpload.resize(start + newSize);
    // While we have resized the buffer to the aligned size, we only need to
    // copy the actual data
    memcpy(dataToUpload.data() + start, data, size);

    const auto offset = currentUsage;

    currentUsage += newSize;

    if (currentUsage >= totalSize) {
        throw_error("Mega buffer ran out of storage");
    }

    return offset;
}

void
Cel::Renderer::MegaBuffer::record_upload_cmd(
    VkCommandBuffer cmd,
    Handle<AllocatedBuffer> staging,
    const RenderGraph::PassServer& server)
{
    if (dataToUpload.empty()) {
        return;
    }

    Utils::upload_to_buffer(cmd,
                            dataToUpload.data(),
                            dataToUpload.size(),
                            buffer,
                            uploadOffset,
                            server.get_resource(staging));

    // Reset
    uploadOffset = UINT32_MAX;
    dataToUpload.clear();
}

void
Cel::Renderer::MegaBuffer::register_upload_to_pass(
    Handle<AllocatedBuffer> stagingHandle,
    RenderGraph::PassBuilder& pass) const
{
    if (!dataToUpload.empty()) {
        pass.upload_buffer(dataToUpload.size(), stagingHandle, buffer.handle);
    }
}
