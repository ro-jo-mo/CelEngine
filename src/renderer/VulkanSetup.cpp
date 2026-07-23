#include "renderer/VulkanSetup.h"

#include "core/Error.h"
#include "renderer/AssetServer.h"
#include "renderer/DeletionQueue.h"
#include "renderer/Descriptors.h"
#include "renderer/PipelineBuilder.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanTypes.h"
#include "renderer/VulkanUtils.h"
#include "renderer/Window.h"
#include "renderer/render-graph/PipelineBuilder.h"

#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

using namespace Cel;
using namespace Cel::Renderer;

constexpr bool useValidationLayers = true;

void
init_vulkan(ResourceManager& resourceManager)
{
    // Firstly create a window
    auto& window = resourceManager.insert_resource<Window>();
    VkSurfaceKHR surface;

    // Create a vulkan instance with our requirements
    vkb::InstanceBuilder builder;
    auto instanceBuild = builder.set_app_name("My App")
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

    VulkanContext context{ .instance = instanceBuild.instance,
                           .gpu = deviceBuild.physical_device,
                           .device = deviceBuild.device,
                           .surface = surface };

    resourceManager.insert_resource(context);

    auto [graphicsQueue, graphicsQueueFamily] =
        deviceBuild.get_queue_and_index(vkb::QueueType::graphics).value();

    GraphicsQueue queue{ graphicsQueue, graphicsQueueFamily };

    resourceManager.insert_resource(queue);

    VmaAllocator allocator;
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = context.gpu;
    allocatorInfo.device = context.device;
    allocatorInfo.instance = context.instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    resourceManager.insert_resource(allocator);

    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

    cleanup->push([=, &window]() {
        vmaDestroyAllocator(allocator);
        vkDestroyDevice(context.device, nullptr);
        vkDestroySurfaceKHR(instanceBuild, context.surface, nullptr);
        vkb::destroy_debug_utils_messenger(context.instance,
                                           instanceBuild.debug_messenger);

        vkDestroyInstance(context.instance, nullptr);
        SDL_DestroyWindow(window->window);
        SDL_Quit();
    });
}

void
init_swapchain(ResourceManager& resourceManager)
{
    auto& context = resourceManager.GetResource<VulkanContext>();

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

    Swapchain swapchain{ .swapchain = swapchainBuild.swapchain,
                         .images = swapchainBuild.get_images().value(),
                         .imageViews = swapchainBuild.get_image_views().value(),
                         .format = format,
                         .extent = swapchainBuild.extent };

    // Create semaphores for swapchain images
    VkSemaphoreCreateInfo semaphoreCreateInfo =
        Initialisers::semaphore_create_info();

    swapchain.submitSemaphores =
        std::vector<VkSemaphore>(swapchain.images.size());

    for (size_t i = 0; i < swapchain.images.size(); i++) {
        vkCreateSemaphore(context->device,
                          &semaphoreCreateInfo,
                          nullptr,
                          &swapchain.submitSemaphores[i]);
    }

    resourceManager.insert_resource(swapchain);

    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

    cleanup->push([=, &context]() {
        vkDestroySwapchainKHR(context->device, swapchain.swapchain, nullptr);
        for (int i = 0; i < swapchain.imageViews.size(); i++) {
            vkDestroyImageView(
                context->device, swapchain.imageViews[i], nullptr);
            vkDestroySemaphore(
                context->device, swapchain.submitSemaphores[i], nullptr);
        }
    });
}

void
init_draw_images(ResourceManager& resourceManager)
{
    auto& swapchain = resourceManager.GetResource<Swapchain>();
    auto& allocator = resourceManager.GetResource<VmaAllocator>();
    auto& context = resourceManager.GetResource<VulkanContext>();

    const VkExtent3D drawExtent{ .width = swapchain->extent.width,
                                 .height = swapchain->extent.height,
                                 .depth = 1 };

    DrawImage drawImage;
    drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    drawImage.imageExtent = drawExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo drawImageCreateInfo = Initialisers::image_create_info(
        drawImage.imageFormat, drawImageUsages, drawImage.imageExtent);

    VmaAllocationCreateInfo drawImageAllocationInfo{};
    drawImageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    drawImageAllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vmaCreateImage(*allocator,
                   &drawImageCreateInfo,
                   &drawImageAllocationInfo,
                   &drawImage.image,
                   &drawImage.allocation,
                   nullptr);
    vmaSetAllocationName(*allocator, drawImage.allocation, "draw_image_alloc");

    VkImageViewCreateInfo drawViewCreateInfo =
        Initialisers::image_view_create_info(
            drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    vk_check(vkCreateImageView(
        context->device, &drawViewCreateInfo, nullptr, &drawImage.imageView));

    resourceManager.insert_resource(drawImage);

    // Create depth image
    DepthImage depth;
    depth.imageFormat = VK_FORMAT_D32_SFLOAT;
    depth.imageExtent = drawImage.imageExtent;
    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageCreateInfo depthImageCreateInfo = Initialisers::image_create_info(
        depth.imageFormat, depthImageUsages, drawImage.imageExtent);

    vmaCreateImage(*allocator,
                   &depthImageCreateInfo,
                   &drawImageAllocationInfo,
                   &depth.image,
                   &depth.allocation,
                   nullptr);

    vmaSetAllocationName(*allocator, depth.allocation, "depth_image_alloc");
    VkImageViewCreateInfo depthImageViewCreateInfo =
        Initialisers::image_view_create_info(
            depth.imageFormat, depth.image, VK_IMAGE_ASPECT_DEPTH_BIT);
    vk_check(vkCreateImageView(
        context->device, &depthImageViewCreateInfo, nullptr, &depth.imageView));

    resourceManager.insert_resource(depth);

    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

    cleanup->push([=, &context, &allocator]() {
        vkDestroyImageView(context->device, drawImage.imageView, nullptr);
        vkDestroyImageView(context->device, depth.imageView, nullptr);
        vmaDestroyImage(*allocator, drawImage.image, drawImage.allocation);
        vmaDestroyImage(*allocator, depth.image, depth.allocation);
    });
}

void
init_frame_data(ResourceManager& resourceManager)
{
    auto& queue = resourceManager.GetResource<GraphicsQueue>();
    auto& context = resourceManager.GetResource<VulkanContext>();
    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

    FrameData frameData{ .totalFrames = FRAME_OVERLAP };

    VkCommandPoolCreateInfo commandPoolCreateInfo =
        Initialisers::command_pool_create_info(
            queue->family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkFenceCreateInfo fenceCreateInfo =
        Initialisers::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo =
        Initialisers::semaphore_create_info();

    // Create per frame resources
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        CurrentFrameData frame{};

        // Firstly create command pool / buffer
        vk_check(vkCreateCommandPool(context->device,
                                     &commandPoolCreateInfo,
                                     nullptr,
                                     &frame.commandPool));

        VkCommandBufferAllocateInfo commandBufferAllocateInfo =
            Initialisers::command_buffer_allocate_info(frame.commandPool, 1);

        vk_check(vkAllocateCommandBuffers(
            context->device, &commandBufferAllocateInfo, &frame.commandBuffer));

        // Next create synchronisation primitives
        vk_check(vkCreateFence(
            context->device, &fenceCreateInfo, nullptr, &frame.renderFence));

        vk_check(vkCreateSemaphore(context->device,
                                   &semaphoreCreateInfo,
                                   nullptr,
                                   &frame.acquireSemaphore));

        frameData.frames.push_back(frame);

        cleanup->push([=, &context]() {
            vkDestroyCommandPool(context->device, frame.commandPool, nullptr);
            vkDestroyFence(context->device, frame.renderFence, nullptr);

            vkDestroySemaphore(
                context->device, frame.acquireSemaphore, nullptr);
        });
    }

    // Lastly create resources for immediate submit
    ImmediateSubmit immediate{};
    vk_check(vkCreateCommandPool(context->device,
                                 &commandPoolCreateInfo,
                                 nullptr,
                                 &immediate.commandPool));

    VkCommandBufferAllocateInfo commandBufferAllocateInfo =
        Initialisers::command_buffer_allocate_info(immediate.commandPool, 1);

    vk_check(vkAllocateCommandBuffers(
        context->device, &commandBufferAllocateInfo, &immediate.commandBuffer));

    vk_check(vkCreateFence(
        context->device, &fenceCreateInfo, nullptr, &immediate.fence));

    resourceManager.insert_resource(immediate);
    resourceManager.insert_resource(frameData);

    cleanup->push([=, &context]() {
        vkDestroyCommandPool(context->device, immediate.commandPool, nullptr);
        vkDestroyFence(context->device, immediate.fence, nullptr);
    });
}

void
init_descriptor_data(ResourceManager& resourceManager)
{
    auto& context = resourceManager.GetResource<VulkanContext>();
    auto& frameData = resourceManager.GetResource<FrameData>();

    GlobalDescriptorData global{};

    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
    };

    global.allocator.init(context->device, 10, sizes);

    // Set scene layout
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        VkDescriptorSetLayoutBindingFlagsCreateInfo bindFlags = {
            .sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
            .pNext = nullptr
        };

        std::array<VkDescriptorBindingFlags, 2> flagArray{
            0,
            VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
        };

        builder.bindings[1].descriptorCount = 4048;

        bindFlags.bindingCount = 2;
        bindFlags.pBindingFlags = flagArray.data();

        global.sceneLayout = builder.build(context->device,
                                           VK_SHADER_STAGE_VERTEX_BIT |
                                               VK_SHADER_STAGE_FRAGMENT_BIT,
                                           &bindFlags);
    }

    // Skybox layout
    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        builder.bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        builder.add_binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        builder.bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        global.skyboxLayout = builder.build(context->device);
    }

    auto& frames = frameData->frames;
    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        // create a descriptor pool
        std::vector<DescriptorAllocator::PoolSizeRatio> frameSizes = {
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
        };

        frames[i].descriptorAllocator.init(context->device, 1000, frameSizes);

        cleanup->push(
            [&, i]() { frames[i].descriptorAllocator.destroy_pools(); });
    }

    auto& globalRes = resourceManager.insert_resource(global);

    cleanup->push([&]() {
        vkDestroyDescriptorSetLayout(
            context->device, globalRes->sceneLayout, nullptr);
        vkDestroyDescriptorSetLayout(
            context->device, globalRes->skyboxLayout, nullptr);
        globalRes->allocator.destroy_pools();
    });
}

void
init_asset_server(ResourceManager& resourceManager)
{
    auto& context = resourceManager.GetResource<VulkanContext>();
    auto& queue = resourceManager.GetResource<GraphicsQueue>();
    auto& allocator = resourceManager.GetResource<VmaAllocator>();
    auto& immediate = resourceManager.GetResource<ImmediateSubmit>();
    auto& global = resourceManager.GetResource<GlobalDescriptorData>();

    resourceManager.insert_resource<AssetServer>(
        context, allocator, immediate, queue, global);
}

void
init_pipeline(ResourceManager& resourceManager)
{
    auto& context = resourceManager.GetResource<VulkanContext>();
    auto& global = resourceManager.GetResource<GlobalDescriptorData>();

    Pipeline meshPipe = PipelineBuilder_(context->device)
                            .add_shader_module("../../shaders/mesh.vert.spv",
                                               VK_SHADER_STAGE_VERTEX_BIT)
                            .add_shader_module("../../shaders/mesh.frag.spv",
                                               VK_SHADER_STAGE_FRAGMENT_BIT)
                            .build();

    resourceManager.insert_resource<MeshPipeline>(meshPipe.pipeline,
                                                  meshPipe.pipelineLayout);

    // Create skybox pipeline

    auto skyboxPipe = PipelineBuilder_(context->device)
                          .add_shader_module("../../shaders/skybox.vert.spv",
                                             VK_SHADER_STAGE_VERTEX_BIT)
                          .add_shader_module("../../shaders/skybox.frag.spv",
                                             VK_SHADER_STAGE_FRAGMENT_BIT)
                          .build();

    resourceManager.insert_resource<SkyboxPipeline>(skyboxPipe.pipeline,
                                                    skyboxPipe.pipelineLayout);

    // clean structures

    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

    cleanup->push([=, &context]() {
        vkDestroyPipelineLayout(
            context->device, meshPipe.pipelineLayout, nullptr);
        vkDestroyPipeline(context->device, meshPipe.pipeline, nullptr);

        vkDestroyPipelineLayout(
            context->device, skyboxPipe.pipelineLayout, nullptr);
        vkDestroyPipeline(context->device, skyboxPipe.pipeline, nullptr);
    });
}

void
VulkanInitialiser::initialise(ResourceManager& resourceManager)
{
    resourceManager.insert_resource<FinalCleanup>();
    resourceManager.insert_resource<RenderExtent>();

    init_vulkan(resourceManager);
    init_swapchain(resourceManager);
    init_draw_images(resourceManager);
    init_frame_data(resourceManager);
    init_descriptor_data(resourceManager);
    init_asset_server(resourceManager);

    init_pipeline(resourceManager);
}
