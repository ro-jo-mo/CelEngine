#include "core/CorePlugin.h"
#include "core/Transform.h"
#include "ecs/Schedule.h"
#include "input/Input.h"

void
Cel::CorePlugin::Build(Scheduler scheduler, ResourceManager& resourceManager)
{
    resourceManager.InsertResource<Input::Input>();

    scheduler.AddSystem(MainUpdate::First, Input::ProcessInputEvents);

    // Propagate the transform hierarchy after startup as well as in update
    // To ensure the first frame works correctly
    scheduler.AddChain(
        MainUpdate::Last, ComputeRootGlobalTransform, HierarchyPropagation);
    scheduler.AddChain(
        Startup::Last, ComputeRootGlobalTransform, HierarchyPropagation);
}