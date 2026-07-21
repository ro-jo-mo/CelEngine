#pragma once

#include "AssetServer.h"
#include "AssetTypes.h"
#include "Camera.h"
#include "VulkanTypes.h"
#include "core/Transform.h"
#include "ecs/System.h"

namespace Cel::Renderer {

// A wrapper around all the data needed for drawing
// Easier to split up rendering code when I don't need to pass 10 arguments
// around
struct DrawData
{
    Query<With<GlobalTransform, Handle<Mesh>, Handle<Material>>>& renderables;
    Query<With<Camera>>& cameras;
    Resource<VulkanContext>& context;
    Resource<Swapchain>& swapchain;
    Resource<GraphicsQueue>& graphicsQueue;
    Resource<DrawImage>& drawImage;
    Resource<DepthImage>& depthImage;
    Resource<MeshPipeline>& meshPipeline;
    Resource<SkyboxPipeline>& skyboxPipeline;
    Resource<RenderExtent>& renderExtent;
    Resource<FrameData>& frameData;
    Resource<AssetServer>& assetServer;
    Resource<GlobalDescriptorData>& globalDescriptors;
    Resource<VmaAllocator>& allocator;
    Resource<ImmediateSubmit>& immediate;

    Camera& camera;
    CurrentFrameData* frame;
    VkCommandBuffer cmd;

    AllocatedBuffer indirectBuffer;
    AllocatedBuffer entityBuffer;

    void draw();

    void cleanup_draw();

    void draw_geometry();

    void create_indirect_data();

    void draw_skybox();

    void bind_scene_data(VkDescriptorSet sceneDescriptor) const;

    void draw_model(GlobalTransform& transform,
                   Handle<Mesh> meshHandle,
                   Handle<Material> matHandle) const;
};

void
draw(Query<With<GlobalTransform, Handle<Mesh>, Handle<Material>>>& renderables,
     Query<With<Camera>>& cameras,
     Resource<VulkanContext>& context,
     Resource<Swapchain>& swapchain,
     Resource<GraphicsQueue>& graphicsQueue,
     Resource<DrawImage>& drawImage,
     Resource<DepthImage>& depthImage,
     Resource<MeshPipeline>& meshPipeline,
     Resource<SkyboxPipeline>& skyboxPipeline,
     Resource<RenderExtent>& renderExtent,
     Resource<FrameData>& frameData,
     Resource<AssetServer>& assetServer,
     Resource<GlobalDescriptorData>& globalDescriptors,
     Resource<VmaAllocator>& allocator,
     Resource<ImmediateSubmit>& immediate);

}
