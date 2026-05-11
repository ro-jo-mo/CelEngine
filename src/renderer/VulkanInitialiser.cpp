#include "renderer/VulkanInitialiser.h"
#include "renderer/DeletionQueue.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/Window.h"
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

using namespace Cel;

constexpr bool useValidationLayers = true;

void
InitVulkan(ResourceManager& resourceManager)
{
    // Firstly create a window
    auto& window = resourceManager.InsertResource<Renderer::Window>();
    VkSurfaceKHR surface;

    // Create a vulkan instance with our requirements
    vkb::InstanceBuilder builder;
    auto instanceResult = builder.set_app_name("My App")
                              .request_validation_layers(useValidationLayers)
                              .use_default_debug_messenger()
                              .require_api_version(1, 3, 0)
                              .build();

    auto instance = instanceResult.value();

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
    auto deviceBuild = deviceBuilder.build().value();

    VkPhysicalDevice gpu = deviceBuild.physical_device;
    VkDevice device = deviceBuild.device;

    resourceManager.InsertResource(instance.instance);
    resourceManager.InsertResource(instance.debug_messenger);
    resourceManager.InsertResource(gpu);
    resourceManager.InsertResource(device);
    resourceManager.InsertResource(surface);

    auto& cleanup = resourceManager.GetResource<Renderer::FinalCleanup>();

    cleanup->Push([=, &window]() {
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkb::destroy_debug_utils_messenger(instance.instance,
                                           instance.debug_messenger);

        vkDestroyInstance(instance.instance, nullptr);
        SDL_DestroyWindow(window->window);
        SDL_Quit();
    });
}

void
InitSwapchain(ResourceManager& resourceManager)
{
    auto& gpu = resourceManager.GetResource<VkPhysicalDevice>();
    auto& device = resourceManager.GetResource<VkDevice>();
    auto& surface = resourceManager.GetResource<VkSurfaceKHR>();

    vkb::SwapchainBuilder builder{ *gpu, *device, *surface };
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

    auto& cleanup = resourceManager.GetResource<Renderer::FinalCleanup>();

    cleanup->Push([=, &gpu, &device, &surface]() {
        vkDestroySwapchainKHR(*device, swapchain.swapchain, nullptr);

        for (int i = 0; i < views.size(); i++) {
            vkDestroyImageView(*device, views[i], nullptr);
        }
    });
}

void
InitTrianglePipeline(ResourceManager& resourceManager)
{
    auto& device = resourceManager.GetResource<VkDevice>();

    VkShaderModule vertShader;
    if (!Renderer::Helpers::LoadShader(
            "../shaders/shader.vert", *device, &vertShader)) {
        throw std::runtime_error("Failed to load vertex shader");
    }

    VkShaderModule fragShader;
    if (!Renderer::Helpers::LoadShader(
            "../shaders/shader.frag", *device, &vertShader)) {
        throw std::runtime_error("Failed to load fragment shader");
    }

    VkPipelineLayout pipelineLayout;

    VkPipelineLayoutCreateInfo pipeline_layout_info =
        vkinit::pipeline_layout_create_info();
    VK_CHECK(vkCreatePipelineLayout(
        *device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout));
}

void
Renderer::VulkanInitialiser::Initialise(ResourceManager& resourceManager)
{
    resourceManager.InsertResource(FinalCleanup{});
    InitVulkan(resourceManager);
    InitSwapchain(resourceManager);
}
