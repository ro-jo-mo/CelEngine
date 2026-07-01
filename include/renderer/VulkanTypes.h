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

struct CurrentFrameData
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

struct FrameData
{
    std::vector<CurrentFrameData> frames;
    size_t currentFrame;
    const size_t totalFrames;
    [[nodiscard]] CurrentFrameData& Get() { return frames[currentFrame]; }
    void Tick() { currentFrame = (currentFrame + 1) % totalFrames; }
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
    glm::mat4 viewProjMatrix;
};

struct GlobalDescriptorData
{
    DescriptorAllocator allocator;

    // Mesh pipeline
    VkDescriptorSetLayout sceneLayout;
    VkDescriptorSetLayout materialLayout;

    // Skybox pipeline
    VkDescriptorSetLayout skyboxLayout;
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

template<typename T>
struct Pipeline
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct GenericTag;
struct DepthTag
{};
struct DrawTag
{};
struct SkyboxTag
{};
struct MeshTag
{};

}
using DepthImage = Detail::AllocatedImage<Detail::DepthTag>;
using DrawImage = Detail::AllocatedImage<Detail::DrawTag>;
using AllocatedImage = Detail::AllocatedImage<Detail::GenericTag>;

using SkyboxPipeline = Detail::Pipeline<Detail::SkyboxTag>;
using MeshPipeline = Detail::Pipeline<Detail::MeshTag>;
}