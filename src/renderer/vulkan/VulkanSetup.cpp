#include "renderer/vulkan/VulkanSetup.h"

#include "../../../include/renderer/asset/AssetServer.h"
#include "../../../include/renderer/components/RenderTypes.h"
#include "../../../include/renderer/components/Window.h"
#include "../../../include/renderer/resource-management/DeletionQueue.h"
#include "../../../include/renderer/resource-management/PipelineBuilder.h"
#include "../../../include/renderer/resource-management/Queues.h"
#include "../../../include/renderer/vulkan/VulkanHelpers.h"
#include "../../../include/renderer/vulkan/VulkanUtils.h"
#include "core/Error.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

using namespace Cel;
using namespace Cel::Renderer;

constexpr bool useValidationLayers = true;

static void
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
                             .enable_extension("VK_EXT_debug_utils")
                             .require_api_version(1, 3, 0)

                             .build()
                             .value();

    Utils::vkSetDebugUtilsObjectName =
        reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
            instanceBuild.fp_vkGetInstanceProcAddr(
                instanceBuild, "vkSetDebugUtilsObjectNameEXT"));

    SDL_Vulkan_CreateSurface(window->window, instanceBuild, nullptr, &surface);

    VkPhysicalDeviceFeatures features10{};
    features10.multiDrawIndirect = true;
    features10.drawIndirectFirstInstance = true;
    features10.shaderSampledImageArrayDynamicIndexing = true;
    features10.shaderInt64 = true;

    VkPhysicalDeviceVulkan11Features features11{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES
    };
    features11.shaderDrawParameters = true;

    VkPhysicalDeviceVulkan12Features features12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
    };
    features12.shaderSampledImageArrayNonUniformIndexing = true;
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

    // Name the queues!
    VkDebugUtilsObjectNameInfoEXT nameInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = VK_OBJECT_TYPE_QUEUE,
        .objectHandle = 0,
        .pObjectName = ""
    };

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
            Queues::graphics.isDedicated = true;

            nameInfo.objectHandle =
                reinterpret_cast<uint64_t>(Queues::graphics.queue);
            nameInfo.pObjectName = "graphics_queue";
            Utils::vkSetDebugUtilsObjectName(context->device, &nameInfo);
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
            Queues::compute.isDedicated = true;

            nameInfo.objectHandle =
                reinterpret_cast<uint64_t>(Queues::compute.queue);
            nameInfo.pObjectName = "compute_queue";
            Utils::vkSetDebugUtilsObjectName(context->device, &nameInfo);
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
            Queues::transfer.isDedicated = true;

            nameInfo.objectHandle =
                reinterpret_cast<uint64_t>(Queues::transfer.queue);
            nameInfo.pObjectName = "transfer_queue";
            Utils::vkSetDebugUtilsObjectName(context->device, &nameInfo);
        }
    }

    if (!graphicsFound) {
        throw_error("Unable to find a sufficient device (gpu)");
    }
    if (!computeFound) {
        fmt::println("No compute queue found");
        Queues::compute = Queues::graphics;
        Queues::compute.isDedicated = false;
    }
    if (!transferFound) {
        fmt::println("No transfer queue found");
        Queues::transfer = Queues::graphics;
        Queues::transfer.isDedicated = false;
    }

    PipelineBuilder::initialise_default_descriptor(context->device);

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

static VkSurfaceFormatKHR
select_unorm_surface_format(VkPhysicalDevice gpu, VkSurfaceKHR surface)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> available(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        gpu, surface, &count, available.data());

    constexpr std::array preferred = { VK_FORMAT_B8G8R8A8_UNORM,
                                       VK_FORMAT_R8G8B8A8_UNORM };

    for (const auto format : preferred) {
        for (const auto& surfaceFormat : available) {
            if (surfaceFormat.format == format &&
                surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return surfaceFormat;
            }
        }
    }

    throw_error("Unable to find a valid UNORM swapchain format");
    return {};
}

static void
create_swapchain(const VulkanContext& context,
                 const VkExtent2D extent,
                 Swapchain& swapchain,
                 const VkSwapchainKHR oldSwapchain)
{
    vkb::SwapchainBuilder builder{ context.gpu,
                                   context.device,
                                   context.surface };

    const VkSurfaceFormatKHR surfaceFormat =
        select_unorm_surface_format(context.gpu, context.surface);

    auto swapchainBuild =
        builder.set_desired_format(surfaceFormat)
            .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(extent.width, extent.height)
            .set_old_swapchain(oldSwapchain)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

    if (swapchainBuild.image_format != surfaceFormat.format ||
        swapchainBuild.color_space != surfaceFormat.colorSpace) {
        throw_error("Swapchain was created with non UNORM format");
    }

    swapchain.swapchain = swapchainBuild.swapchain;
    swapchain.images = swapchainBuild.get_images().value();
    swapchain.imageViews = swapchainBuild.get_image_views().value();
    swapchain.format = swapchainBuild.image_format;
    swapchain.extent = swapchainBuild.extent;
    swapchain.imageIndex = 0;
    swapchain.imageAcquired = false;
    swapchain.needsRecreate = false;

    // Create semaphores for swapchain images
    VkSemaphoreCreateInfo semaphoreCreateInfo =
        Initialisers::semaphore_create_info();

    swapchain.presentSemaphores.resize(swapchain.images.size());

    for (size_t i = 0; i < swapchain.images.size(); i++) {
        vkCreateSemaphore(context.device,
                          &semaphoreCreateInfo,
                          nullptr,
                          &swapchain.presentSemaphores[i]);
        Utils::set_resource_name(
            context.device,
            reinterpret_cast<uint64_t>(swapchain.presentSemaphores[i]),
            VK_OBJECT_TYPE_SEMAPHORE,
            fmt::format("present_semaphore_{}", i).c_str());
    }
}

// Destroys the image views and semaphores belonging to the swapchain. The
// swapchain handle itself is left alive, so it can be passed as the old
// swapchain during recreation
static void
destroy_swapchain_resources(const VulkanContext& context, Swapchain& swapchain)
{
    for (const auto view : swapchain.imageViews) {
        vkDestroyImageView(context.device, view, nullptr);
    }
    for (const auto semaphore : swapchain.presentSemaphores) {
        vkDestroySemaphore(context.device, semaphore, nullptr);
    }

    swapchain.imageViews.clear();
    swapchain.images.clear();
    swapchain.presentSemaphores.clear();
}

static void
init_swapchain(Resource<VulkanContext>& context,
               Resource<Window>& window,
               Resource<Swapchain>& swapchain,
               Resource<FinalCleanup>& cleanup)
{
    swapchain.initialise();

    create_swapchain(
        *context, window->get_pixel_extent(), *swapchain, VK_NULL_HANDLE);

    // Capture the underlying objects, the swapchain may be recreated many
    // times before this runs, so it must destroy whatever is current
    VulkanContext* ctx = context.get();
    Swapchain* sc = swapchain.get();

    cleanup->push([ctx, sc]() {
        destroy_swapchain_resources(*ctx, *sc);
        vkDestroySwapchainKHR(ctx->device, sc->swapchain, nullptr);
    });
}

void
Renderer::recreate_swapchain_if_needed(Resource<VulkanContext>& context,
                                       Resource<Window>& window,
                                       Resource<Swapchain>& swapchain)
{
    if (!window->resized && !swapchain->needsRecreate) {
        return;
    }

    // Can't create a zero sized swapchain. Keep the old one and try again
    // once the window is visible
    const VkExtent2D extent = window->get_pixel_extent();
    if (window->minimized || extent.width == 0 || extent.height == 0) {
        return;
    }

    // Nothing may be using the old swapchain images or semaphores
    vkDeviceWaitIdle(context->device);

    const VkSwapchainKHR old = swapchain->swapchain;

    destroy_swapchain_resources(*context, *swapchain);
    create_swapchain(*context, extent, *swapchain, old);

    vkDestroySwapchainKHR(context->device, old, nullptr);

    window->resized = false;
}

void
Renderer::initialise_renderer(Resource<VulkanContext>& context,
                              Resource<VmaAllocator>& allocator,
                              Resource<Window>& window,
                              Resource<Swapchain>& swapchain,
                              Resource<VulkanResourceManager>& manager,
                              Resource<Asset::AssetServer>& server,
                              Resource<Asset::SharedAssetData>& shared,
                              Resource<FinalCleanup>& cleanup)
{
    init_vulkan(context, allocator, window, cleanup);
    init_swapchain(context, window, swapchain, cleanup);
    manager.initialise(context->device, *allocator);
    server.initialise(*manager, shared);
}
