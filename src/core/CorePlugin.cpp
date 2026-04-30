#include "core/CorePlugin.h"
#include "core/Transform.h"

void
Cel::CorePlugin::Build(Scheduler scheduler, ResourceManager& resourceManager)
{
    scheduler.AddSystem<HierarchyPropagation>(FinalUpdate);
}