#include "renderer/RenderPlugin.h"

#include "renderer/CleanupRenderer.h"
#include "renderer/VulkanSetup.h"
#include "renderer/Window.h"

using namespace Cel;

void
Renderer::RenderPlugin::Build(Scheduler scheduler,
                              ResourceManager& resourceManager)
{
    VulkanInitialiser::Initialise(resourceManager);
    scheduler.AddSystem<CleanupRenderer>(Schedule::Cleanup);
    scheduler.AddSystem<WindowSystem>(Schedule::Render);
}
