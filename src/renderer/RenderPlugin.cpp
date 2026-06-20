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
    scheduler.AddSystem(TearDown::Middle, CleanupRenderer);
    scheduler.AddSystem(Render::Update, WindowSystem);
    scheduler.AddSystem(Render::Update, SetRenderExtent);
    scheduler.AddSystem(Render::PostUpdate, Draw);
}
