#include "Test.h"

#include "../include/core/App.h"
#include "core/CorePlugin.h"
#include "renderer/RenderPlugin.h"

using namespace Cel;

int
main()
{
    App ecs;
    ecs.AddPlugin<CorePlugin>().AddPlugin<Renderer::RenderPlugin>();
    ecs.Start<Startup>()
        .Loop<FixedSchedule<PhysicsUpdate, 50>,
              DynamicSchedule<MainUpdate>,
              DynamicSchedule<Render>>()
        .End<TearDown>();

    return 0;
}