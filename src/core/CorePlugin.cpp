#include "core/CorePlugin.h"
#include "core/Transform.h"
#include "ecs/Schedule.h"
#include "input/Input.h"

void
Cel::CorePlugin::Build(Scheduler scheduler, ResourceManager& resourceManager)
{
    resourceManager.InsertResource<Input::Input>();

    scheduler.AddSystem(MainUpdate::First, Input::ProcessInputEvents);

    // There are several places we need to propagate the hierarchy
    // Effectively anyplace where we expect changes to a transform to be finalized
    scheduler.AddChain(
        MainUpdate::Last, ComputeRootGlobalTransform, HierarchyPropagation);
    scheduler.AddChain(
        PhysicsUpdate::Last, ComputeRootGlobalTransform, HierarchyPropagation);
    scheduler.AddChain(
        Startup::Last, ComputeRootGlobalTransform, HierarchyPropagation);
}