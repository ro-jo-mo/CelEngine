#include "core/CorePlugin.h"
#include "core/Transform.h"
#include "ecs/Schedule.h"

void
Cel::CorePlugin::Build(Scheduler scheduler, ResourceManager& resourceManager)
{
    scheduler.AddSystem(MainUpdate::Last, HierarchyPropagation);
    // Ensures the very first frame works correctly (physics runs before
    // propagation)
    scheduler.AddChain(
        Startup::Last, ComputeRootGlobalTransform, HierarchyPropagation);
}