#include "core/CorePlugin.h"
#include "core/Transform.h"
#include "ecs/Schedule.h"

void
Cel::CorePlugin::Build(Scheduler scheduler, ResourceManager& resourceManager)
{
    scheduler.AddSystem(MainUpdate::Last, HierarchyPropagation);
}