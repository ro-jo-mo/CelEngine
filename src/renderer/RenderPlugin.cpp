#include "renderer/RenderPlugin.h"
#include "renderer/Window.h"
void
Cel::Renderer::RenderPlugin::Build(Scheduler scheduler,
                                   ResourceManager& resourceManager)
{
    resourceManager.InsertResource<Window>();
}
