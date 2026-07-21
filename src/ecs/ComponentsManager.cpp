#include "ecs/ComponentsManager.h"

#include "ecs/ComponentArray.h"
#include "ecs/Types.h"

#include <ranges>

using namespace Cel;

void
ComponentsManager::destroy_entity(const Entity entity)
{
    for (const auto& componentArrayPtr : componentArrays | std::views::values) {
        componentArrayPtr->destroy_entity(entity);
    }
}
