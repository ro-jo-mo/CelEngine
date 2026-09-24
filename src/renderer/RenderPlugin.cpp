#include "renderer/RenderPlugin.h"

#include "renderer/Camera.h"
#include "renderer/CleanupRenderer.h"
#include "renderer/VulkanSetup.h"
#include "renderer/Window.h"
#include "renderer/render-graph/RenderGraphPlugin.h"
#include "renderer/resource-management/VulkanResourceManager.h"

using namespace Cel;

void
Renderer::RenderPlugin::build(SystemScheduler scheduler,
                              ResourceManager& resourceManager)
{
    resourceManager.insert_resource<FinalCleanup>();
    resourceManager.insert_resource<RenderExtent>();
    resourceManager.insert_resource_as_null<Assets::AssetServer>();
    resourceManager.insert_resource_as_null<VulkanResourceManager>();
    resourceManager.insert_resource_as_null<Swapchain>();
    resourceManager.insert_resource_as_null<Window>();
    resourceManager.insert_resource_as_null<VulkanContext>();
    resourceManager.insert_resource_as_null<VmaAllocator>();

    scheduler.add_system(Startup::PreStart, initialise_renderer);

    scheduler.add_group(Render::First, set_render_extent, camera_system);

    scheduler.add_chain(
        TearDown::Middle, cleanup_asset_server, cleanup_renderer);
}
