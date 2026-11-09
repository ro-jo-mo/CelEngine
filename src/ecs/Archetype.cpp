#include "ecs/Archetype.h"

#include <cassert>

Cel::Archetype::Archetype(
    const std::array<ComponentInfo, MAX_COMPONENTS>& componentInfos,
    const Signature& signature)
    : componentInfos(componentInfos)
{
    for (size_t i = 0; i < MAX_COMPONENTS; i++) {
        if (signature.test(i)) {

            // Add mapping to table
            componentToIndex[i] = storages.size();

            auto& info = componentInfos[i];

            // Create storage for this component

            storages.push_back({ .info = info,
                                 .size = info.size,
                                 .align = info.align,
                                 // What a terrible syntax
                                 // Should get an allocation that matches are
                                 // alignment. Imagine if msvc actually
                                 // supported the spec smh
                                 .alloc = static_cast<char*>(::operator new(
                                     info.size * MAX_ENTITIES,
                                     std::align_val_t{ info.align })) });
        }
    }
}

Cel::Archetype::~Archetype()
{
    for (const auto& store : storages) {

        // If not trivially destructible, we must actually execute the
        // destructor
        if (store.info.destructor != nullptr) {
            store.info.destructor(store.alloc, totalEntities);
        }

        // Lastly, free the allocation
        ::operator delete(store.alloc, std::align_val_t{ store.info.align });
    }
}

void*
Cel::Archetype::get_component(const Entity entity, const ComponentId component)
{
    assert(entityToIndex.contains(entity));

    return get_component_from_entity_index(
        entityToIndex.at(entity), storages[componentToIndex[component]]);
}

void*
Cel::Archetype::get_component_from_entity_index(const size_t index,
                                                const ComponentStorage& store)
{
    return store.alloc + store.size * index;
}

void
Cel::Archetype::delete_entity(const Entity entity)
{
    assert(entityToIndex.contains(entity));

    const auto index = entityToIndex[entity];
    // Swap the last element of the array to the deleted components place
    // This keeps the array tightly packed
    const auto last = --totalEntities;

    for (const auto& store : storages) {
        auto position = get_component_from_entity_index(index, store);

        if (store.info.destructor != nullptr) {
            store.info.destructor(get_component_from_entity_index(index, store),
                                  1);
        }

        // Swap with the last entity
        auto lastPosition = get_component_from_entity_index(last, store);

        if (store.info.move != nullptr) {
            store.info.move(lastPosition, position);
        } else {
            memcpy(position, lastPosition, store.size);
        }
    }

    // Update the mapping for the entity we're swapping
    auto swappedEntity = indexToEntity[last];

    entityToIndex[swappedEntity] = index;
    indexToEntity[index] = swappedEntity;

    // Lastly remove this entity from the map
    entityToIndex.erase(entity);
}