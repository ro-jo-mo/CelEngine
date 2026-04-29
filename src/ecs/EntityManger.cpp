#include "ecs/EntityManager.h"

using namespace Cel;

Entity EntityManager::AllocateEntity() {
	if (toReuse.empty()) {
		return entityCounter++;
	}
	const auto entity = toReuse.front();
	toReuse.pop();
	return entity;
}

void EntityManager::DestroyEntity(const Entity entity) {
	toReuse.push(entity);
}
