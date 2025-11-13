#include "ecs/Ecs.h"

#include <iostream>

#include "ecs/Time.h"
#include "ecs/World.h"

using namespace Cel;

void Ecs::Run() {
    resourceManager.InitialiseGroups();
    // Startup
    schedules[PreStartup].Execute();
    schedules[Startup].Execute();
    schedules[PostStartup].Execute();

    auto [time, world] = resourceManager.GetResources<Time, World>();
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
        // lastly render

        // update time
        world->Flush();
        systemManager.UpdateViews();
        time->Tick();
    }
}


