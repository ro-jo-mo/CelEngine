#include "renderer/RenderPlugin.h"

#include "../../include/renderer/resource-management/CleanupRenderer.h"
#include "../../include/renderer/vulkan/VulkanSetup.h"
#include "renderer/Camera.h"
#include "renderer/Window.h"
#include "renderer/render-graph/RenderGraphPlugin.h"
#include "renderer/resource-management/VulkanResourceManager.h"

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

    manager.add_group(Render::First, set_render_extent, camera_system);

    manager.add_chain(
        TearDown::Middle, cleanup_asset_server, cleanup_renderer);
}
