#include "ecs/World.h"

using namespace Cel;

void
World::Destroy(const Entity entity)
{
    toDestroy.push_back(entity);
}

void
World::AddChild(Entity parent, Entity child)
{
    // If child has no parent, add component
    // If parent has no children, add component
    //
}

bool
World::Flush()
{
    // order
    // add
    // remove
    // destroy
    for (const auto& cmd : toAdd) {
        cmd->Execute();
    }
    for (const auto& cmd : toRemove) {
        cmd->Execute();
    }
    for (const auto& ent : toDestroy) {
        ExecuteDestroy(ent);
    }
    const auto changesMade =
        toAdd.size() + toRemove.size() + toDestroy.size() > 0;
    toAdd.clear();
    toRemove.clear();
    toDestroy.clear();
    return changesMade;
}

void
World::ExecuteDestroy(Entity entity) const
{
    entityManager.DestroyEntity(entity);
    componentsManager.DestroyEntity(entity);
}
