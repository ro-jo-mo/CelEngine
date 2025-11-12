#include "ecs/ResourceManager.h"

void Cel::ResourceManager::Queue(const std::shared_ptr<IResourceGroup> &group) {
    toInitialise.push(group);
}

void Cel::ResourceManager::InitialiseGroups() {
    while (!toInitialise.empty()) {
        const auto &group = toInitialise.front();
        toInitialise.pop();
        group->Initialise(*this);
    }
}
