#include "ecs/ComponentsManager.h"

#include "ecs/ComponentArray.h"
#include "ecs/Types.h"

#include <ranges>

using namespace Cel;

void
ComponentsManager::DestroyEntity(const Entity entity)
{
    for (const auto& componentArrayPtr : componentArrays | std::views::values) {
        componentArrayPtr->DestroyEntity(entity);
    }
}
