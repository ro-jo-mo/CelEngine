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
#include <SDL3/SDL_vulkan.h>
#include <VkBootstrap.h>

using namespace Cel;
using namespace Cel::Renderer;

constexpr bool useValidationLayers = true;

void
InitVulkan(ResourceManager& resourceManager)
{
    // Firstly create a window
    auto& window = resourceManager.InsertResource<Window>();
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

    VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
    };
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    vkb::PhysicalDeviceSelector selector{ instanceBuild };
    vkb::PhysicalDevice physicalDevice =
        selector.set_minimum_version(1, 3)
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

    resourceManager.InsertResource(context);

    auto graphicsQueue =
        deviceBuild.get_queue(vkb::QueueType::graphics).value();
    auto graphicsQueueFamily =
        deviceBuild.get_queue_index(vkb::QueueType::graphics).value();

    GraphicsQueue queue{ graphicsQueue, graphicsQueueFamily };

    resourceManager.InsertResource(queue);

    VmaAllocator allocator;
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = context.gpu;
    allocatorInfo.device = context.device;
    allocatorInfo.instance = context.instance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    resourceManager.InsertResource(allocator);

    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

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
        Initialisers::SemaphoreCreateInfo();

    swapchain.submitSemaphores =
        std::vector<VkSemaphore>(swapchain.images.size());

    for (size_t i = 0; i < swapchain.images.size(); i++) {
        vkCreateSemaphore(context->device,
                          &semaphoreCreateInfo,
                          nullptr,
                          &swapchain.submitSemaphores[i]);
    }

    resourceManager.InsertResource(swapchain);

    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

    cleanup->Push([=, &context]() {
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
InitDrawImages(ResourceManager& resourceManager)
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

    VkImageCreateInfo drawImageCreateInfo = Initialisers::ImageCreateInfo(
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
        Initialisers::ImageViewCreateInfo(
            drawImage.imageFormat, drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    VkCheck(vkCreateImageView(
        context->device, &drawViewCreateInfo, nullptr, &drawImage.imageView));

    resourceManager.InsertResource(drawImage);

    // Create depth image
    DepthImage depth;
    depth.imageFormat = VK_FORMAT_D32_SFLOAT;
    depth.imageExtent = drawImage.imageExtent;
    VkImageUsageFlags depthImageUsages{};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageCreateInfo depthImageCreateInfo = Initialisers::ImageCreateInfo(
        depth.imageFormat, depthImageUsages, drawImage.imageExtent);

    vmaCreateImage(*allocator,
                   &depthImageCreateInfo,
                   &drawImageAllocationInfo,
                   &depth.image,
                   &depth.allocation,
                   nullptr);

    vmaSetAllocationName(*allocator, depth.allocation, "depth_image_alloc");
    VkImageViewCreateInfo depthImageViewCreateInfo =
        Initialisers::ImageViewCreateInfo(
            depth.imageFormat, depth.image, VK_IMAGE_ASPECT_DEPTH_BIT);
    VkCheck(vkCreateImageView(
        context->device, &depthImageViewCreateInfo, nullptr, &depth.imageView));

    resourceManager.InsertResource(depth);

    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

    cleanup->Push([=, &context, &allocator]() {
        vkDestroyImageView(context->device, drawImage.imageView, nullptr);
        vkDestroyImageView(context->device, depth.imageView, nullptr);
        vmaDestroyImage(*allocator, drawImage.image, drawImage.allocation);
        vmaDestroyImage(*allocator, depth.image, depth.allocation);
    });
}

void
InitFrameData(ResourceManager& resourceManager)
{
    auto& queue = resourceManager.GetResource<GraphicsQueue>();
    auto& context = resourceManager.GetResource<VulkanContext>();
    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

    FrameData frameData{ .totalFrames = FRAME_OVERLAP };

    VkCommandPoolCreateInfo commandPoolCreateInfo =
        Initialisers::CommandPoolCreateInfo(
            queue->family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    VkFenceCreateInfo fenceCreateInfo =
        Initialisers::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo =
        Initialisers::SemaphoreCreateInfo();

    // Create per frame resources
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        CurrentFrameData frame{};

        // Firstly create command pool / buffer
        VkCheck(vkCreateCommandPool(context->device,
                                    &commandPoolCreateInfo,
                                    nullptr,
                                    &frame.commandPool));

        VkCommandBufferAllocateInfo commandBufferAllocateInfo =
            Initialisers::CommandBufferAllocateInfo(frame.commandPool, 1);

        VkCheck(vkAllocateCommandBuffers(
            context->device, &commandBufferAllocateInfo, &frame.commandBuffer));

        // Next create synchronisation primitives
        VkCheck(vkCreateFence(
            context->device, &fenceCreateInfo, nullptr, &frame.renderFence));

        VkCheck(vkCreateSemaphore(context->device,
                                  &semaphoreCreateInfo,
                                  nullptr,
                                  &frame.acquireSemaphore));

        frameData.frames.push_back(frame);

        cleanup->Push([=, &context]() {
            vkDestroyCommandPool(context->device, frame.commandPool, nullptr);
            vkDestroyFence(context->device, frame.renderFence, nullptr);

            vkDestroySemaphore(
                context->device, frame.acquireSemaphore, nullptr);
        });
    }

    // Lastly create resources for immediate submit
    ImmediateSubmit immediate{};
    VkCheck(vkCreateCommandPool(context->device,
                                &commandPoolCreateInfo,
                                nullptr,
                                &immediate.commandPool));

    VkCommandBufferAllocateInfo commandBufferAllocateInfo =
        Initialisers::CommandBufferAllocateInfo(immediate.commandPool, 1);

    VkCheck(vkAllocateCommandBuffers(
        context->device, &commandBufferAllocateInfo, &immediate.commandBuffer));

    VkCheck(vkCreateFence(
        context->device, &fenceCreateInfo, nullptr, &immediate.fence));

    resourceManager.InsertResource(immediate);
    resourceManager.InsertResource(frameData);

    cleanup->Push([=, &context]() {
        vkDestroyCommandPool(context->device, immediate.commandPool, nullptr);
        vkDestroyFence(context->device, immediate.fence, nullptr);
    });
}

void
InitDescriptorData(ResourceManager& resourceManager)
{
    auto& context = resourceManager.GetResource<VulkanContext>();
    auto& frameData = resourceManager.GetResource<FrameData>();

    GlobalDescriptorData global{};

    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
    };

    global.allocator.Init(context->device, 10, sizes);

    // Set material layout
    {
        DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        global.materialLayout = builder.Build(context->device,
                                              VK_SHADER_STAGE_VERTEX_BIT |
                                                  VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    // Set scene layout
    {
        DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
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

        global.sceneLayout = builder.Build(context->device,
                                           VK_SHADER_STAGE_VERTEX_BIT |
                                               VK_SHADER_STAGE_FRAGMENT_BIT,
                                           &bindFlags);
    }

    // Skybox layout
    {
        DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        builder.bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        builder.bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        global.skyboxLayout = builder.Build(context->device);
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

        frames[i].descriptorAllocator.Init(context->device, 1000, frameSizes);

        cleanup->Push(
            [&, i]() { frames[i].descriptorAllocator.DestroyPools(); });
    }

    auto& globalRes = resourceManager.InsertResource(global);

    cleanup->Push([&]() {
        vkDestroyDescriptorSetLayout(
            context->device, globalRes->sceneLayout, nullptr);
        vkDestroyDescriptorSetLayout(
            context->device, globalRes->materialLayout, nullptr);
        vkDestroyDescriptorSetLayout(
            context->device, globalRes->skyboxLayout, nullptr);
        globalRes->allocator.DestroyPools();
    });
}

void
InitAssetServer(ResourceManager& resourceManager)
{
    auto& context = resourceManager.GetResource<VulkanContext>();
    auto& queue = resourceManager.GetResource<GraphicsQueue>();
    auto& allocator = resourceManager.GetResource<VmaAllocator>();
    auto& immediate = resourceManager.GetResource<ImmediateSubmit>();
    auto& global = resourceManager.GetResource<GlobalDescriptorData>();

    resourceManager.InsertResource<AssetServer>(
        context, allocator, immediate, queue, global);
}

void
InitPipeline(ResourceManager& resourceManager)
{
    auto& context = resourceManager.GetResource<VulkanContext>();
    auto& drawImage = resourceManager.GetResource<DrawImage>();
    auto& depthImage = resourceManager.GetResource<DepthImage>();
    auto& global = resourceManager.GetResource<GlobalDescriptorData>();

    VkShaderModule meshVert;
    if (!Utils::LoadShader(
            "../../shaders/mesh.vert.spv", context->device, &meshVert)) {
        ThrowError("Failed to load vertex shader");
    }

    VkShaderModule meshFrag;
    if (!Utils::LoadShader(
            "../../shaders/mesh.frag.spv", context->device, &meshFrag)) {
        ThrowError("Failed to load vertex shader");
    }

    // Push constants include the entities global transform, as well as a 3x3
    // normal transform matrix (stored as mat4 for alignment)
    VkPushConstantRange matrixRange = {};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(EntityPushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayout layouts[] = { global->sceneLayout,
                                        global->materialLayout };

    VkPipelineLayoutCreateInfo meshLayoutInfo =
        Initialisers::PipelineLayoutCreateInfo();
    meshLayoutInfo.setLayoutCount = 2;
    meshLayoutInfo.pSetLayouts = layouts;
    meshLayoutInfo.pPushConstantRanges = &matrixRange;
    meshLayoutInfo.pushConstantRangeCount = 1;

    VkPipelineLayout meshLayout;
    VkCheck(vkCreatePipelineLayout(
        context->device, &meshLayoutInfo, nullptr, &meshLayout));

    // #####################

    PipelineBuilder pipelineBuilder;

    // use the triangle layout we created
    pipelineBuilder.pipelineLayout = meshLayout;
    pipelineBuilder.SetVertexInputNone();
    // connecting the vertex and pixel shaders to the pipeline
    pipelineBuilder.SetShaders(meshVert, meshFrag);
    // it will draw triangles
    pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // filled triangles
    pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
    // no backface culling
    pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    // no multisampling
    pipelineBuilder.SetMutisamplingNone();
    // Enable blending later once I figure it out
    pipelineBuilder.DisableBlending();
    // enable depth test
    pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // connect the image format we will draw into, from draw image
    pipelineBuilder.SetColourAttachment(drawImage->imageFormat);
    pipelineBuilder.SetDepthAttachment(depthImage->imageFormat);

    // finally build the pipeline
    auto meshPipeline = pipelineBuilder.BuildPipeline(context->device);

    resourceManager.InsertResource<MeshPipeline>(meshPipeline, meshLayout);

    // Create skybox pipeline

    VkShaderModule skyboxVert;
    if (!Utils::LoadShader(
            "../../shaders/skybox.vert.spv", context->device, &skyboxVert)) {
        ThrowError("Failed to load skybox vert shader");
    }

    VkShaderModule skyboxFrag;
    if (!Utils::LoadShader(
            "../../shaders/skybox.frag.spv", context->device, &skyboxFrag)) {
        ThrowError("Failed to load skybox frag shader");
    }

    VkPipelineLayoutCreateInfo skyboxLayoutCreateInfo =
        Initialisers::PipelineLayoutCreateInfo();
    skyboxLayoutCreateInfo.setLayoutCount = 1;
    skyboxLayoutCreateInfo.pSetLayouts = &global->skyboxLayout;

    VkPipelineLayout skyboxLayout;

    vkCreatePipelineLayout(
        context->device, &skyboxLayoutCreateInfo, nullptr, &skyboxLayout);

    pipelineBuilder.pipelineLayout = skyboxLayout;
    pipelineBuilder.SetShaders(skyboxVert, skyboxFrag);
    pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
    pipelineBuilder.SetVertexInputFloatArray();

    auto skyboxPipeline = pipelineBuilder.BuildPipeline(context->device);

    resourceManager.InsertResource<SkyboxPipeline>(skyboxPipeline,
                                                   skyboxLayout);

    // clean structures
    vkDestroyShaderModule(context->device, meshFrag, nullptr);
    vkDestroyShaderModule(context->device, meshVert, nullptr);
    vkDestroyShaderModule(context->device, skyboxVert, nullptr);
    vkDestroyShaderModule(context->device, skyboxFrag, nullptr);

    auto& cleanup = resourceManager.GetResource<FinalCleanup>();

    cleanup->Push([=, &context]() {
        vkDestroyPipelineLayout(context->device, meshLayout, nullptr);
        vkDestroyPipeline(context->device, meshPipeline, nullptr);

        vkDestroyPipelineLayout(context->device, skyboxLayout, nullptr);
        vkDestroyPipeline(context->device, skyboxPipeline, nullptr);
    });
}

void
VulkanInitialiser::Initialise(ResourceManager& resourceManager)
{
    auto& c = resourceManager.InsertResource<FinalCleanup>();
    resourceManager.InsertResource<RenderExtent>();

    InitVulkan(resourceManager);
    InitSwapchain(resourceManager);
    InitDrawImages(resourceManager);
    InitFrameData(resourceManager);
    InitDescriptorData(resourceManager);
    InitAssetServer(resourceManager);

    InitPipeline(resourceManager);
}
