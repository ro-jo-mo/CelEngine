#include "renderer/resource-management/MegaBuffer.h"

#include "renderer/resource-management/VulkanResourceManager.h"

Cel::Renderer::PerFrameMegaBuffer::PerFrameMegaBuffer(const uint32_t alignment)
    : alignment(alignment)
{
}

uint32_t
Cel::Renderer::PerFrameMegaBuffer::allocate(const void* data,
                                            const uint32_t size)
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

    return offset;
}

uint32_t
Cel::Renderer::PerFrameMegaBuffer::current_upload_size() const
{
    return dataToUpload.size();
}

void
Cel::Renderer::PerFrameMegaBuffer::push_to_gpu(const AllocatedBuffer& buffer,
                                               VkCommandBuffer cmd,
                                               const AllocatedBuffer& staging)
{
    Utils::upload_to_buffer(cmd,
                            dataToUpload.data(),
                            dataToUpload.size(),
                            buffer,
                            uploadOffset,
                            staging);

    // Reset
    uploadOffset = UINT32_MAX;
    dataToUpload.clear();
}

Cel::Renderer::MegaBuffer::MegaBuffer(const BufferRequirements req,
                                      const std::string& name,
                                      VulkanResourceManager& manager,
                                      const uint32_t alignment)
    : handle(manager.get_handle_from_requirements(req, name))
    , buffer(manager.get_resource_from_handle(handle))
    , alignment(alignment)
{
}

void
Cel::Renderer::MegaBuffer::push_to_gpu(VkCommandBuffer cmd,
                                       const AllocatedBuffer& staging)
{
    PerFrameMegaBuffer::push_to_gpu(buffer, cmd, staging);
}
