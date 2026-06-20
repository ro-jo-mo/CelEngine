#pragma once

#include "DeletionQueue.h"
#include "Descriptors.h"

#include <glm/glm.hpp>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {

constexpr unsigned int FRAME_OVERLAP = 2;

struct Swapchain
{
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkSemaphore> submitSemaphores;
    VkFormat format;
    VkExtent2D extent;
};

struct MeshPipeline
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct GraphicsQueue
{
    VkQueue queue;
    uint32_t family;
};

struct VulkanContext
{
    VkInstance instance;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkSurfaceKHR surface;
};

struct FrameData
{
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore acquireSemaphore;
    VkFence renderFence;
    DescriptorAllocator descriptorAllocator;
    PerFrameCleanup toDelete;
};

struct ImmediateSubmit
{
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFence fence;
};

struct CurrentFrameData
{
    std::vector<FrameData> frames;
    size_t currentFrame;
    const size_t totalFrames;
    [[nodiscard]] FrameData Get() const { return frames[currentFrame]; }
    void Update() { currentFrame = (currentFrame + 1) % totalFrames; }
};

struct RenderExtent
{
    VkExtent2D extent;
    float renderScale = 1.0f;
};

// Constants unique to each entity
struct EntityPushConstants
{
    glm::mat4 transform;
    // glm::mat4 normalTransform;
    VkDeviceAddress vertexBuffer;
};

struct SceneData
{
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
};

struct GlobalDescriptorData
{
    DescriptorAllocator allocator;
    VkDescriptorSetLayout sceneLayout;
    VkDescriptorSetLayout materialLayout;
};

struct AllocatedBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct AllocatedMeshBuffer
{
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
    uint32_t indexCount;
};

namespace Detail {

template<typename T>
struct AllocatedImage
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct GenericTag;
struct DepthTag
{};
struct DrawTag
{};

}
using DepthImage = Detail::AllocatedImage<Detail::DepthTag>;
using DrawImage = Detail::AllocatedImage<Detail::DrawTag>;
using AllocatedImage = Detail::AllocatedImage<Detail::GenericTag>;
}