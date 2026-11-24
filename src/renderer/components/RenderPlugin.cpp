#include "../../../include/renderer/components/RenderPlugin.h"

#include "renderer/components/Camera.h"
#include "renderer/components/Window.h"
#include "renderer/render-graph/RenderGraphPlugin.h"
#include "renderer/resource-management/CleanupRenderer.h"
#include "renderer/resource-management/VulkanResourceManager.h"
#include "renderer/vulkan/VulkanSetup.h"

using namespace Cel;

void
Renderer::RenderPlugin::build(PluginManager& manager)
{
    manager.insert_resource<FinalCleanup>();
    manager.insert_resource<RenderExtent>();
    manager.insert_resource_as_null<Asset::AssetServer>();
    manager.insert_resource_as_null<Asset::SharedAssetData>();
    manager.insert_resource_as_null<VulkanResourceManager>();
    manager.insert_resource_as_null<Swapchain>();
    manager.insert_resource_as_null<Window>();
    manager.insert_resource_as_null<VulkanContext>();
    manager.insert_resource_as_null<VmaAllocator>();

    manager.add_system(Startup::PreStart, initialise_renderer);

    // Rebuild the swapchain first, so the render extent and every pass
    // registered this frame see the new size
    manager.add_system(Render::First, recreate_swapchain_if_needed);
    manager.add_group(Render::First, set_render_extent, camera_system)
        .after(recreate_swapchain_if_needed);

    manager.add_chain(TearDown::Middle, cleanup_asset_server, cleanup_renderer);
}
