#include "Test.h"

#include "core/CorePlugin.h"
#include "ecs/App.h"
#include "renderer/RenderPlugin.h"

using namespace Cel;

int
main()
{
    App ecs;
    ecs.AddPlugin<CorePlugin>().AddPlugin<Renderer::RenderPlugin>();
    ecs.Run();

    return 0;
}