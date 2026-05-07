#pragma once

#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {

class VulkanEngine
{
  public:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;

  private:
    void InitVulkan();
    void InitSwapchain();
    void InitCommands();
    void InitSyncStructures();
};
}