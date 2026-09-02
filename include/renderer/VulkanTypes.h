#pragma once

#include "Descriptors.h"
#include "common/Handle.h"
#include "resource-management/DeletionQueue.h"

#include <glm/glm.hpp>
#include <vector>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {

constexpr unsigned int FRAMES_IN_FLIGHT = 2;
constexpr unsigned int QUEUE_COUNT = 3;

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

struct Queue
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

struct FramesInFlight
{
    std::vector<FrameData> frames;
    size_t currentFrame;
    const size_t totalFrames;
    [[nodiscard]] FrameData& Get() { return frames[currentFrame]; }
    void Tick() { currentFrame = (currentFrame + 1) % totalFrames; }
};

struct ImmediateSubmit
{
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFence fence;
};

struct RenderExtent
{
    VkExtent2D extent;
    float renderScale = 1.0f;
};

// Constants unique to each entity
struct PerEntityGpuData
{
    glm::mat4 transform;
    glm::mat4 normalTransform;
    uint32_t materialIndex;
};



struct GlobalDescriptorData
{
    DescriptorAllocator allocator;

    // Mesh pipeline
    VkDescriptorSetLayout sceneLayout;

    // Skybox pipeline
    VkDescriptorSetLayout skyboxLayout;
};

struct AllocatedBuffer
{
    Handle<AllocatedBuffer> handle;
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};

struct AllocatedImage
{
    Handle<AllocatedImage> handle;
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct AllocatedMeshBuffer
{
    AllocatedBuffer indexBuffer;
    AllocatedBuffer vertexBuffer;
    VkDeviceAddress vertexBufferAddress;
    uint32_t indexCount;
};

struct BufferRequirements
{
    size_t allocSize;
    VkBufferUsageFlags usages;
    VmaMemoryUsage memoryUsage;
};

struct ImageRequirements
{
    VkFormat format;
    VkExtent3D extent;
    VkImageUsageFlags usages;
    VkImageAspectFlags aspects;
};

struct BufferAccess
{
    VkAccessFlags2 access = VK_ACCESS_2_NONE;
    VkPipelineStageFlags2 stages = VK_PIPELINE_STAGE_2_NONE;
    uint32_t queue = VK_QUEUE_FAMILY_IGNORED;
};

struct ImageAccess
{
    VkAccessFlags2 access = VK_ACCESS_2_NONE;
    VkPipelineStageFlags2 stages = VK_PIPELINE_STAGE_2_NONE;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t queue = VK_QUEUE_FAMILY_IGNORED;
};

struct Pipeline
{
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    std::vector<VkDescriptorSetLayout> descriptorSets;
};

}
