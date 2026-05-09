#include "renderer/VulkanInitialiser.h"

#include "renderer/DeletionQueue.h"
#include "renderer/Window.h"
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

using namespace Cel;

constexpr bool useValidationLayers = true;

void
InitVulkan(ResourceManager& resourceManager)
{
    // Firstly create a window
    auto window = resourceManager.InsertResource<Renderer::Window>();
    VkSurfaceKHR surface;

    // Create a vulkan instance with our requirements
    vkb::InstanceBuilder builder;
    auto instanceResult = builder.set_app_name("My App")
                              .request_validation_layers(useValidationLayers)
                              .use_default_debug_messenger()
                              .require_api_version(1, 3, 0)
                              .build();

    const auto instance = instanceResult.value();

    SDL_Vulkan_CreateSurface(window->window, instance, nullptr, &surface);

    VkPhysicalDeviceVulkan13Features features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
    };
    features.dynamicRendering = true;
    features.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
    };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector{ instance };
    vkb::PhysicalDevice physicalDevice =
        selector.set_minimum_version(1, 3)
            .set_required_features_13(features)
            .set_required_features_12(features12)
            .set_surface(surface)
            .select()
            .value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };

    VkPhysicalDevice gpu = deviceBuilder.build().value().physical_device;
    VkDevice device = deviceBuilder.build().value().device;

    resourceManager.InsertResource(instance.instance);
    resourceManager.InsertResource(instance.debug_messenger);
    resourceManager.InsertResource(gpu);
    resourceManager.InsertResource(device);
    resourceManager.InsertResource(surface);

    auto cleanup = resourceManager.GetResource<Renderer::FinalCleanup>();

    cleanup->Push([&]() {
        vkDestroyDevice(device, nullptr);

        vkb::destroy_debug_utils_messenger(instance.instance,
                                           instance.debug_messenger);
        vkDestroyInstance(instance.instance, nullptr);
        SDL_DestroyWindow(window->window);
    });
}

void
InitSwapchain(ResourceManager& resourceManager)
{
    VkPhysicalDevice gpu =
        resourceManager.GetResource<VkPhysicalDevice>().resource;
    VkDevice device = resourceManager.GetResource<VkDevice>().resource;
    VkSurfaceKHR surface = resourceManager.GetResource<VkSurfaceKHR>().resource;

    vkb::SwapchainBuilder builder{ gpu, device, surface };
    auto format = VK_FORMAT_R8G8B8A8_UNORM;

    auto swapchain = builder
                         .set_desired_format(VkSurfaceFormatKHR{
                             .format = format,
                             .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
                         .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                         .set_desired_extent(1600, 900)
                         .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                         .build()
                         .value();

    auto views = swapchain.get_image_views().value();

    resourceManager.InsertResource(swapchain.swapchain);
    resourceManager.InsertResource(views);
    resourceManager.InsertResource(swapchain.get_images().value());

    auto cleanup = resourceManager.GetResource<Renderer::FinalCleanup>();

    cleanup->Push([&]() {
        vkDestroySwapchainKHR(device, swapchain.swapchain, nullptr);

        for (int i = 0; i < views.size(); i++) {
            vkDestroyImageView(device, views[i], nullptr);
        }
    });
}

void
Renderer::VulkanInitialiser::Initialise(ResourceManager& resourceManager)
{
    resourceManager.InsertResource(FinalCleanup{});
    InitVulkan(resourceManager);
}
