#include "core/CorePlugin.h"
#include "core/Transform.h"
#include "ecs/Schedule.h"

void
Cel::CorePlugin::Build(Scheduler scheduler, ResourceManager& resourceManager)
{
    // Propagate the transform hierarchy after startup as well as in update
    // To ensure the first frame works correctly
    scheduler.AddChain(
        MainUpdate::Last, ComputeRootGlobalTransform, HierarchyPropagation);
    scheduler.AddChain(
        Startup::Last, ComputeRootGlobalTransform, HierarchyPropagation);
}