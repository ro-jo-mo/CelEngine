#include "core/CorePlugin.h"
#include "core/Transform.h"
#include "ecs/Schedule.h"
#include "input/Input.h"

void
Cel::CorePlugin::build(PluginManager& manager)
{
    manager.insert_resource<Input::Input>();

    manager.add_system(MainUpdate::First, Input::process_input_events);

    // There are several places we need to propagate the hierarchy
    // Effectively anyplace where we expect changes to a transform to be
    // finalized
    manager.add_chain(
        MainUpdate::Last, compute_root_global_transform, hierarchy_propagation);
    manager.add_chain(PhysicsUpdate::Last,
                        compute_root_global_transform,
                        hierarchy_propagation);
    manager.add_chain(
        Startup::Last, compute_root_global_transform, hierarchy_propagation);
}