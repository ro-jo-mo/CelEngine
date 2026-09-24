#include "renderer/VulkanSetup.h"

#include "../../include/renderer/resource-management/DeletionQueue.h"
#include "../../include/renderer/resource-management/PipelineBuilder.h"
#include "core/Error.h"
#include "renderer/AssetServer.h"
#include "renderer/Descriptors.h"
#include "renderer/Queues.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanTypes.h"
#include "renderer/VulkanUtils.h"
#include "renderer/Window.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

using namespace Cel;
using namespace Cel::Renderer;

constexpr bool useValidationLayers = true;

void
init_vulkan(Resource<VulkanContext>& context,
            Resource<VmaAllocator>& allocator,
            Resource<Window>& window,
            Resource<FinalCleanup>& cleanup)
{
    window.initialise();

    // Firstly create a window
    VkSurfaceKHR surface;

    // Create a vulkan instance with our requirements
    vkb::InstanceBuilder builder;
    auto instanceBuild = builder.set_app_name("Cel App")
                             .request_validation_layers(useValidationLayers)
                             .use_default_debug_messenger()
                             .require_api_version(1, 3, 0)
                             .build()
                             .value();

    SDL_Vulkan_CreateSurface(window->window, instanceBuild, nullptr, &surface);

    VkPhysicalDeviceFeatures features10{};
    features10.multiDrawIndirect = true;

    VkPhysicalDeviceVulkan11Features features11{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES
    };
    features11.shaderDrawParameters = true;

    VkPhysicalDeviceVulkan12Features features12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
    };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    features12.descriptorBindingPartiallyBound = true;
    features12.descriptorBindingVariableDescriptorCount = true;
    features12.runtimeDescriptorArray = true;
    features12.drawIndirectCount = true;
    features12.scalarBlockLayout = true;
    features12.timelineSemaphore = true;

    VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
    };
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    vkb::PhysicalDeviceSelector selector{ instanceBuild };
    vkb::PhysicalDevice physicalDevice =
        selector.set_minimum_version(1, 3)
            .set_required_features(features10)
            .set_required_features_11(features11)
            .set_required_features_12(features12)
            .set_required_features_13(features13)
            .set_surface(surface)
            .select()
            .value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    auto deviceBuild = deviceBuilder.build().value();

    context.initialise(instanceBuild.instance,
                       deviceBuild.physical_device,
                       deviceBuild.device,
                       surface);

    bool computeFound = false;
    bool transferFound = false;
    bool graphicsFound = false;

    // Originally I assumed it wouldn't really matter if we picked a random
    // queue that matched the filter requirements below.
    // However, most vendors seem to order their queues such that the first
    // found is likely the ideal match, i.e. the rtx 3070 has graphics, transfer
    // & compute first, then specialised video and optical flow queues that
    // would also match the transfer filter

    // As such, we go for the first match when searching for queues

    for (const auto& [i, queue] :
         std::views::enumerate(physicalDevice.get_queue_families())) {

        // Graphics
        if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (graphicsFound) {
                continue;
            }
            graphicsFound = true;
            vkGetDeviceQueue(context->device, i, 0, &Queues::graphics.queue);
            Queues::graphics.family = i;
        }

        // Compute
        if (queue.queueFlags & VK_QUEUE_COMPUTE_BIT &&
            !(queue.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            if (computeFound) {
                continue;
            }
            computeFound = true;
            vkGetDeviceQueue(context->device, i, 0, &Queues::compute.queue);
            Queues::compute.family = i;
        }

        // Transfer
        if (queue.queueFlags & VK_QUEUE_TRANSFER_BIT &&
            !(queue.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            !(queue.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            if (transferFound) {
                continue;
            }
            transferFound = true;
            vkGetDeviceQueue(context->device, i, 0, &Queues::transfer.queue);
            Queues::transfer.family = i;
        }
    }

    if (!graphicsFound) {
        throw_error("Unable to find a sufficient device (gpu)");
    }
    if (!computeFound) {
        Queues::compute = Queues::graphics;
    }
    if (!transferFound) {
        Queues::transfer = Queues::graphics;
    }

    allocator.initialise();

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = context->gpu;
    allocatorInfo.device = context->device;
    allocatorInfo.instance = context->instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, allocator.get());

    cleanup->push([&]() {
        vmaDestroyAllocator(*allocator);
        vkDestroyDevice(context->device, nullptr);
        vkDestroySurfaceKHR(instanceBuild, context->surface, nullptr);
        vkb::destroy_debug_utils_messenger(context->instance,
                                           instanceBuild.debug_messenger);

        vkDestroyInstance(context->instance, nullptr);
        SDL_DestroyWindow(window->window);
        SDL_Quit();
    });
}

void
init_swapchain(Resource<VulkanContext>& context,
               Resource<Swapchain>& swapchain,
               Resource<FinalCleanup>& cleanup)
{
    vkb::SwapchainBuilder builder{ context->gpu,
                                   context->device,
                                   context->surface };
    auto format = VK_FORMAT_R8G8B8A8_UNORM;

    auto swapchainBuild =
        builder
            .set_desired_format(VkSurfaceFormatKHR{
                .format = format,
                .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(1600, 900)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

    swapchain.initialise(swapchainBuild.swapchain,
                         swapchainBuild.get_images().value(),
                         swapchainBuild.get_image_views().value(),
                         format,
                         swapchainBuild.extent);

    // Create semaphores for swapchain images
    VkSemaphoreCreateInfo semaphoreCreateInfo =
        Initialisers::semaphore_create_info();

    swapchain->submitSemaphores.resize(swapchain->images.size());

    for (size_t i = 0; i < swapchain->images.size(); i++) {
        vkCreateSemaphore(context->device,
                          &semaphoreCreateInfo,
                          nullptr,
                          &swapchain->submitSemaphores[i]);
    }

    cleanup->push([&]() {
        vkDestroySwapchainKHR(context->device, swapchain->swapchain, nullptr);
        for (int i = 0; i < swapchain->imageViews.size(); i++) {
            vkDestroyImageView(
                context->device, swapchain->imageViews[i], nullptr);
            vkDestroySemaphore(
                context->device, swapchain->submitSemaphores[i], nullptr);
        }
    });
}

void
Renderer::initialise_renderer(Resource<VulkanContext>& context,
                              Resource<VmaAllocator>& allocator,
                              Resource<Window>& window,
                              Resource<Swapchain>& swapchain,
                              Resource<VulkanResourceManager>& manager,
                              Resource<Assets::AssetServer>& server,
                              Resource<FinalCleanup>& cleanup)
{
    init_vulkan(context, allocator, window, cleanup);
    init_swapchain(context, swapchain, cleanup);
    manager.initialise(context->device, *allocator);
    server.initialise(*manager);
}
