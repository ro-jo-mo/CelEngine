#include "ecs/ComponentsManager.h"

#include <ranges>

#include "ecs/ComponentArray.h"
#include "ecs/Types.h"

using namespace Cel;

void
ComponentsManager::DestroyEntity(const Entity entity) {
  for (const auto &componentArrayPtr: componentArrays | std::views::values) {
    componentArrayPtr->DestroyEntity(entity);
  }
}
