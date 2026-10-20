#pragma once

#include "ecs/ResourceManager.h"
#include "passes/HandleAllocator.h"
#include "resource-management/DeletionQueue.h"

namespace Cel::Renderer {
namespace Asset {
class AssetServer;
}
class VulkanResourceManager;
struct Window;
struct VulkanContext;

void
initialise_renderer(Resource<VulkanContext>& context,
                    Resource<VmaAllocator>& allocator,
                    Resource<Window>& window,
                    Resource<Swapchain>& swapchain,
                    Resource<VulkanResourceManager>& manager,
                    Resource<Asset::AssetServer>& server,
                    Resource<FinalCleanup>& cleanup);
}