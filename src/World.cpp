#include "ecs/World.h"

using namespace Cel;

void
World::Destroy(const Entity entity) const {
  entityManager.DestroyEntity(entity);
  componentsManager->DestroyEntity(entity);
}

bool
World::Flush() {
  // order
  // add
  // remove
  // destroy
  for (const auto &cmd: toAdd) {
    cmd->Execute();
  }
  for (const auto &cmd: toRemove) {
    cmd->Execute();
  }
  for (const auto &ent: toDestroy) {
    Destroy(ent);
  }
  const auto changesMade = toAdd.size() + toRemove.size() + toDestroy.size() > 0;
  toAdd.clear();
  toRemove.clear();
  toDestroy.clear();
  return changesMade;
}
