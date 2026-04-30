#include "ecs/Ecs.h"

#include <iostream>

#include "ecs/Time.h"
#include "ecs/World.h"

using namespace Cel;

void
Ecs::Run()
{
    // Startup
    schedules[PreStartup].Execute();
    schedules[Startup].Execute();
    schedules[PostStartup].Execute();

    auto& time = resourceManager.GetResource<Resource<Time>>();
    auto& world = resourceManager.GetResource<Resource<World>>();

    // Update loop
    while (true) {
        // run fixed update first
        time->SwitchToFixed();

        while (time->FixedUpdateRequired()) {
            schedules[PreFixedUpdate].Execute();
            schedules[FixedUpdate].Execute();
            schedules[PostFixedUpdate].Execute();
            time->FixedTick();
        }

        // then update
        time->SwitchToDynamic();
        schedules[PreUpdate].Execute();
        schedules[Update].Execute();
        schedules[PostUpdate].Execute();
        schedules[FinalUpdate].Execute();

        schedules[Render].Execute();

        // update time
        world->Flush();
        queryManager.UpdateQueries();

        time->Tick();
    }
}
