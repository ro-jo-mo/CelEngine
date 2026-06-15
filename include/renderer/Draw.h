#pragma once

#include "AssetServer.h"
#include "AssetTypes.h"
#include "Camera.h"
#include "VulkanTypes.h"
#include "core/Transform.h"
#include "ecs/System.h"

namespace Cel::Renderer {

class Draw
    : public System<
          Query<With<GlobalTransform, Handle<Mesh>, Handle<Material>>>,
          Query<With<Camera>>,
          Resource<VulkanContext>,
          Resource<Swapchain>,
          Resource<GraphicsQueue>,
          Resource<DrawImage>,
          Resource<DepthImage>,
          Resource<MeshPipeline>,
          Resource<RenderExtent>,
          Resource<CurrentFrameData>,
          Resource<AssetServer>,
          Resource<GlobalDescriptorData>>
{
  public:
    void Run(Query<With<GlobalTransform, Handle<Mesh>, Handle<Material>>>&
                 renderables,
             Query<With<Camera>>& cameras,
             Resource<VulkanContext>& context,
             Resource<Swapchain>& swapchain,
             Resource<GraphicsQueue>& graphicsQueue,
             Resource<DrawImage>& drawImage,
             Resource<DepthImage>& depthImage,
             Resource<MeshPipeline>& pipeline,
             Resource<RenderExtent>& renderExtent,
             Resource<CurrentFrameData>& currentFrameData,
             Resource<AssetServer>& assetServer,
             Resource<GlobalDescriptorData>& globalDescriptors) override;
};

class SetRenderExtent
    : public System<Resource<RenderExtent>,
                    Resource<DrawImage>,
                    Resource<Swapchain>>
{
  public:
    void Run(Resource<RenderExtent>&,
             Resource<DrawImage>&,
             Resource<Swapchain>&) override;
};
}
