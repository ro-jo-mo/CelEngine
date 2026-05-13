#include "renderer/VulkanSetup.h"
#include "renderer/DeletionQueue.h"
#include "renderer/PipelineBuilder.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanTypes.h"
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
    auto instanceBuild = builder.set_app_name("My App")
                             .request_validation_layers(useValidationLayers)
                             .use_default_debug_messenger()
                             .require_api_version(1, 3, 0)
                             .build()
                             .value();

    SDL_Vulkan_CreateSurface(window->window, instanceBuild, nullptr, &surface);

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

    vkb::PhysicalDeviceSelector selector{ instanceBuild };
    vkb::PhysicalDevice physicalDevice =
        selector.set_minimum_version(1, 3)
            .set_required_features_13(features)
            .set_required_features_12(features12)
            .set_surface(surface)
            .select()
            .value();

    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    auto deviceBuild = deviceBuilder.build().value();

    Renderer::VulkanContext context{ .instance = instanceBuild.instance,
                                     .gpu = deviceBuild.physical_device,
                                     .device = deviceBuild.device,
                                     .surface = surface };

    resourceManager.InsertResource<>(context);

    auto graphicsQueue =
        deviceBuild.get_queue(vkb::QueueType::graphics).value();
    auto graphicsQueueFamily =
        deviceBuild.get_queue_index(vkb::QueueType::graphics).value();

    Renderer::GraphicsQueue queue{ graphicsQueue, graphicsQueueFamily };

    resourceManager.InsertResource<>(queue);

    VmaAllocator allocator;
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = context.gpu;
    allocatorInfo.device = context.device;
    allocatorInfo.instance = context.instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    resourceManager.InsertResource<>(allocator);

    auto& cleanup = resourceManager.GetResource<Renderer::FinalCleanup>();

    cleanup->Push([=, &window]() {
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
InitSwapchain(ResourceManager& resourceManager)
{
    auto& context = resourceManager.GetResource<Renderer::VulkanContext>();

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

    Renderer::Swapchain swapchain{
        .swapchain = swapchainBuild.swapchain,
        .images = swapchainBuild.get_images().value(),
        .imageViews = swapchainBuild.get_image_views().value(),
        .format = format,
        .extent = swapchainBuild.extent,
    };

    resourceManager.InsertResource(swapchain);

    auto& cleanup = resourceManager.GetResource<Renderer::FinalCleanup>();

    cleanup->Push([=, &context]() {
        vkDestroySwapchainKHR(context->device, swapchain.swapchain, nullptr);

        for (int i = 0; i < swapchain.imageViews.size(); i++) {
            vkDestroyImageView(
                context->device, swapchain.imageViews[i], nullptr);
        }
    });
}

void
InitDrawImages(ResourceManager& resourceManager)
{
    auto& swapchain = resourceManager.GetResource<Renderer::Swapchain>();
    auto& allocator = resourceManager.GetResource<VmaAllocator>();
    auto& context = resourceManager.GetResource<Renderer::VulkanContext>();

    VkExtent3D drawExtent{ .width = swapchain->extent.width,
                           .height = swapchain->extent.height,
                           .depth = 1 };

    Renderer::DrawImage drawImage;
    drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo drawImageCreateInfo =
        Renderer::Initialisers::ImageCreateInfo(
            drawImage.imageFormat, drawImageUsages, drawExtent);

    VmaAllocationCreateInfo drawImageAllocationInfo{};
    drawImageAllocationInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    drawImageAllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    vmaCreateImage(*allocator,
                   &drawImageCreateInfo,
                   &drawImageAllocationInfo,
                   &drawImage.image,
                   &drawImage.allocation,
                   nullptr);
    VkImageViewCreateInfo drawViewCreateInfo =
        Renderer::Initialisers::ImageViewCreateInfo(
            drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    Renderer::VkCheck(vkCreateImageView(
        context->device, &drawViewCreateInfo, nullptr, &drawImage.imageView));

    auto& cleanup = resourceManager.GetResource<Renderer::FinalCleanup>();

    cleanup->Push([=, &context, &allocator]() {
        vkDestroyImageView(context->device, drawImage.imageView, nullptr);
        vmaDestroyImage(*allocator, drawImage.image, drawImage.allocation);
    });
}

void
InitTrianglePipeline(ResourceManager& resourceManager)
{
    auto& context = resourceManager.GetResource<Renderer::VulkanContext>();
    auto& drawImage = resourceManager.GetResource<Renderer::DrawImage>();

    VkShaderModule vertShader;
    if (!Renderer::Helpers::LoadShader(
            "../shaders/shader.vert.spv", context->device, &vertShader)) {
        throw std::runtime_error("Failed to load vertex shader");
    }

    VkShaderModule fragShader;
    if (!Renderer::Helpers::LoadShader(
            "../shaders/shader.frag.spv", context->device, &fragShader)) {
        throw std::runtime_error("Failed to load fragment shader");
    }

    VkPipelineLayout pipelineLayout;

    VkPipelineLayoutCreateInfo pipeline_layout_info =
        Cel::Renderer::Initialisers::PipelineLayoutCreateInfo();
    Renderer::VkCheck(vkCreatePipelineLayout(
        context->device, &pipeline_layout_info, nullptr, &pipelineLayout));

    Renderer::PipelineBuilder pipelineBuilder;

    // use the triangle layout we created
    pipelineBuilder.pipelineLayout = pipelineLayout;
    // connecting the vertex and pixel shaders to the pipeline
    pipelineBuilder.SetShaders(vertShader, fragShader);
    // it will draw triangles
    pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // filled triangles
    pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    // no backface culling
    pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    // no multisampling
    pipelineBuilder.SetMutisamplingNone();
    // no blending
    pipelineBuilder.DisableBlending();
    // no depth testing
    pipelineBuilder.DisableDepthTest();

    // connect the image format we will draw into, from draw image
    pipelineBuilder.SetColourAttachment(drawImage->imageFormat);
    pipelineBuilder.SetDepthAttachment(VK_FORMAT_UNDEFINED);

    // finally build the pipeline
    auto pipeline = pipelineBuilder.BuildPipeline(context->device);

    resourceManager.InsertResource(pipeline);

    // clean structures
    vkDestroyShaderModule(context->device, fragShader, nullptr);
    vkDestroyShaderModule(context->device, vertShader, nullptr);

    auto& cleanup = resourceManager.GetResource<Renderer::FinalCleanup>();

    cleanup->Push([=, &context]() {
        vkDestroyPipelineLayout(context->device, pipelineLayout, nullptr);
        vkDestroyPipeline(context->device, pipeline, nullptr);
    });
}

void
InitFrameData(ResourceManager& resourceManager)
{
    auto& queue = resourceManager.GetResource<Renderer::GraphicsQueue>();
    auto& context = resourceManager.GetResource<Renderer::VulkanContext>();
    auto& cleanup = resourceManager.GetResource<Renderer::FinalCleanup>();

    Renderer::CurrentFrameData currentFrameData{ .totalFrames =
                                                     Renderer::FRAME_OVERLAP };

    VkCommandPoolCreateInfo commandPoolCreateInfo =
        Renderer::Initialisers::CommandPoolCreateInfo(
            queue->graphicsFamily,
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkFenceCreateInfo fenceCreateInfo =
        Renderer::Initialisers::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo =
        Renderer::Initialisers::SemaphoreCreateInfo();

    // Create per frame resources
    for (int i = 0; i < Renderer::FRAME_OVERLAP; i++) {
        Renderer::FrameData frameData{};

        // Firstly create command pool / buffer
        Renderer::VkCheck(vkCreateCommandPool(context->device,
                                              &commandPoolCreateInfo,
                                              nullptr,
                                              &frameData.commandPool));

        VkCommandBufferAllocateInfo commandBufferAllocateInfo =
            Renderer::Initialisers::CommandBufferAllocateInfo(
                frameData.commandPool, 1);

        Renderer::VkCheck(vkAllocateCommandBuffers(context->device,
                                                   &commandBufferAllocateInfo,
                                                   &frameData.commandBuffer));

        // Next create synchronisation primtiives
        Renderer::VkCheck(vkCreateFence(context->device,
                                        &fenceCreateInfo,
                                        nullptr,
                                        &frameData.renderFence));

        Renderer::VkCheck(vkCreateSemaphore(context->device,
                                            &semaphoreCreateInfo,
                                            nullptr,
                                            &frameData.renderSemaphore));

        Renderer::VkCheck(vkCreateSemaphore(context->device,
                                            &semaphoreCreateInfo,
                                            nullptr,
                                            &frameData.swapchainSemaphore));

        currentFrameData.frames.push_back(frameData);

        cleanup->Push([=, &context]() {
            vkDestroyCommandPool(
                context->device, frameData.commandPool, nullptr);
            vkDestroyFence(context->device, frameData.renderFence, nullptr);
            vkDestroySemaphore(
                context->device, frameData.renderSemaphore, nullptr);
        });
    }

    // Lastly create resources for immediate submit
    Renderer::ImmediateSubmit immediate{};
    Renderer::VkCheck(vkCreateCommandPool(context->device,
                                          &commandPoolCreateInfo,
                                          nullptr,
                                          &immediate.commandPool));

    VkCommandBufferAllocateInfo commandBufferAllocateInfo =
        Renderer::Initialisers::CommandBufferAllocateInfo(immediate.commandPool,
                                                          1);

    Renderer::VkCheck(vkAllocateCommandBuffers(
        context->device, &commandBufferAllocateInfo, &immediate.commandBuffer));

    Renderer::VkCheck(vkCreateFence(
        context->device, &fenceCreateInfo, nullptr, &immediate.fence));

    cleanup->Push([=, &context]() {
        vkDestroyCommandPool(context->device, immediate.commandPool, nullptr);
        vkDestroyFence(context->device, immediate.fence, nullptr);
    });
}

void
Renderer::VulkanInitialiser::Initialise(ResourceManager& resourceManager)
{
    resourceManager.InsertResource(FinalCleanup{});
    resourceManager.InsertResource<RenderExtent>();
    InitVulkan(resourceManager);
    InitSwapchain(resourceManager);
    InitDrawImages(resourceManager);
    InitFrameData(resourceManager);
    InitTrianglePipeline(resourceManager);
}
