#include "ecs/EntityManager.h"

using namespace Cel;

Entity
EntityManager::allocate_entity()
{
    if (toReuse.empty()) {
        return entityCounter++;
    }

    const auto entity = toReuse.front();
    toReuse.pop();
    return entity;
}

void
EntityManager::destroy_entity(const Entity entity)
{
    //   toReuse.push(entity);
}
