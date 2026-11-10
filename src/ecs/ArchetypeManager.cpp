#include "ecs/ArchetypeManager.h"

Cel::ArchetypeManager::ArchetypeManager()
{
    // We start by simply making an empty archetype
    // The rest of then generated dynamically

    auto empty = Signature{};

    const auto& ptr = archetypes.emplace_back(
        std::make_unique<Archetype>(componentInfos, empty));

    sigToArchetype.emplace(empty, ptr.get());
}

void
Cel::ArchetypeManager::move_entity(
    const Entity entity,
    const Signature& previousSig,
    const Signature& newSig,
    const std::unordered_map<ComponentId, void*>& addedComponents)
{
    // Basic idea with the code is, copy all of the previous archetype first.
    // Then look at a map for the rest

    if (!sigToArchetype.contains(newSig)) {
        const auto& ptr = archetypes.emplace_back(
            std::make_unique<Archetype>(componentInfos, newSig));

        sigToArchetype.emplace(newSig, ptr.get());
    }

    auto& archetype = *sigToArchetype.at(newSig);
    auto& oldArchetype = *sigToArchetype.at(previousSig);

    archetype.register_entity(entity);

    // We only need to move components that exist on both
    auto conjunction = previousSig & newSig;

    // Add the new components as well
    // Only the components that exist in the new signature but not the old
    auto toAdd = ~previousSig & newSig;

    for (size_t i = 0; i < MAX_COMPONENTS; i++) {
        if (conjunction.test(i)) {
            archetype.move_component(
                entity, i, oldArchetype.get_component(entity, i));
        }

        if (toAdd.test(i)) {
            archetype.move_component(entity, i, addedComponents.at(i));
        }
    }

    // Delete from the previous archetype now
    oldArchetype.delete_entity(entity);

    update_entity_data(entity, newSig);
}

void
Cel::ArchetypeManager::update_entity_data(const Entity entity,
                                          const Signature& signature)
{
    auto& data = entityDatas[entity];

    auto& arch = *sigToArchetype.at(signature);

    data.archetype = arch;
    data.index = arch.get_entity_index(entity);
}
