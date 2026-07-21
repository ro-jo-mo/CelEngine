#include "renderer/RenderPlugin.h"

#include "renderer/CleanupRenderer.h"
#include "renderer/Draw.h"
#include "renderer/VulkanSetup.h"
#include "renderer/Window.h"

using namespace Cel;

void
Renderer::RenderPlugin::build(Scheduler scheduler,
                              ResourceManager& resourceManager)
{
    VulkanInitialiser::initialise(resourceManager);

    scheduler.add_group(Render::Update, set_render_extent, camera_system);

    scheduler.add_system(Render::PostUpdate, draw);

    scheduler.add_chain(TearDown::Middle, cleanup_asset_server, cleanup_renderer);
}
