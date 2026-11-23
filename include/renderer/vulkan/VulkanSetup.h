#pragma once

#include "../asset/AssetTypes.h"
#include "../passes/HandleAllocator.h"
#include "../resource-management/DeletionQueue.h"
#include "core/App.h"
#include "ecs/ResourceManager.h"

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
                    Resource<Asset::SharedAssetData>& shared,
                    Resource<FinalCleanup>& cleanup);

// Must run before set_render_extent.
void
recreate_swapchain_if_needed(Resource<VulkanContext>& context,
                             Resource<Window>& window,
                             Resource<Swapchain>& swapchain);
}