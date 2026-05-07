#include "Test.h"

#include "core/CorePlugin.h"
#include "ecs/Ecs.h"
#include "renderer/RenderPlugin.h"

using namespace Cel;

int
main()
{
    Ecs ecs;
    ecs.AddPlugin<CorePlugin>().AddPlugin<Renderer::RenderPlugin>();
    ecs.Run();

    return 0;
}