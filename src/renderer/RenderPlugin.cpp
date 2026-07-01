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

    scheduler.AddGroup(Render::Update, SetRenderExtent, CameraSystem);

    scheduler.AddSystem(Render::PostUpdate, Draw);

    scheduler.AddChain(TearDown::Middle, CleanupAssetServer, CleanupRenderer);
}
