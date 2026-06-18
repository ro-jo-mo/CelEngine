#pragma once

#include "AssetServer.h"
#include "AssetTypes.h"
#include "Camera.h"
#include "VulkanTypes.h"
#include "core/Transform.h"
#include "ecs/System.h"

namespace Cel::Renderer {

void Draw(Query<With<GlobalTransform, Handle<Mesh>, Handle<Material>>>&
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
             Resource<GlobalDescriptorData>& globalDescriptors);


void
SetRenderExtent(Resource<RenderExtent>& renderExtent,
                Resource<DrawImage>& drawImage,
                Resource<Swapchain>& swapchain);
}
