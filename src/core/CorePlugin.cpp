#include "core/CorePlugin.h"
#include "core/Transform.h"
#include "ecs/Schedule.h"
#include "input/Input.h"

void
Cel::CorePlugin::build(Scheduler scheduler, ResourceManager& resourceManager)
{
    resourceManager.insert_resource<Input::Input>();

    scheduler.add_system(MainUpdate::First, Input::process_input_events);

    // There are several places we need to propagate the hierarchy
    // Effectively anyplace where we expect changes to a transform to be finalized
    scheduler.add_chain(
        MainUpdate::Last, compute_root_global_transform, hierarchy_propagation);
    scheduler.add_chain(
        PhysicsUpdate::Last, compute_root_global_transform, hierarchy_propagation);
    scheduler.add_chain(
        Startup::Last, compute_root_global_transform, hierarchy_propagation);
}