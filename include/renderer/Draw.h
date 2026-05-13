#pragma once
#include "VulkanTypes.h"
#include "ecs/System.h"

namespace Cel::Renderer {

class DrawGeometry
    : public System<Resource<VulkanContext>,
                    Resource<DrawImage>,
                    Resource<VkPipeline>,
                    Resource<RenderExtent>>

{
  public:
    void Run(Resource<VulkanContext>& context,
             Resource<DrawImage>& drawImage,
             Resource<VkPipeline>& pipeline,
             Resource<RenderExtent>& renderExtent) override;
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