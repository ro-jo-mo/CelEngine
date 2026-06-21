#include "renderer/RenderPlugin.h"

#include "renderer/CleanupRenderer.h"
#include "renderer/Draw.h"
#include "renderer/VulkanSetup.h"
#include "renderer/Window.h"

using namespace Cel;

void
Renderer::RenderPlugin::Build(Scheduler scheduler,
                              ResourceManager& resourceManager)
{
    VulkanInitialiser::Initialise(resourceManager);

    scheduler.AddChain(TearDown::Middle, CleanupAssetServer, CleanupRenderer);

    scheduler.AddSystem(Render::Update, WindowSystem);
    scheduler.AddSystem(Render::Update, SetRenderExtent);

    scheduler.AddChain(Render::PostUpdate, Draw, CleanupAfterDraw);
}
