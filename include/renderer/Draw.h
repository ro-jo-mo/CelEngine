#pragma once
#include "VulkanTypes.h"
#include "ecs/System.h"

namespace Cel::Renderer {

class Draw
    : public System<Resource<VulkanContext>,
                    Resource<Swapchain>,
                    Resource<GraphicsQueue>,
                    Resource<DrawImage>,
                    Resource<MeshPipeline>,
                    Resource<RenderExtent>,
                    Resource<CurrentFrameData>>

{
  public:
    void Run(Resource<VulkanContext>& context,
             Resource<Swapchain>& swapchain,
             Resource<GraphicsQueue>& graphicsQueue,
             Resource<DrawImage>& drawImage,
             Resource<MeshPipeline>& pipeline,
             Resource<RenderExtent>& renderExtent,
             Resource<CurrentFrameData>& frameData) override;
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