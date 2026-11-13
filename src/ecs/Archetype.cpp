#include "ecs/Archetype.h"

#include <cassert>

Cel::Archetype::Archetype(
    const std::array<ComponentInfo, MAX_COMPONENTS>& componentInfos,
    const Signature& signature)
    : signature(signature)
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

void
Cel::ComponentInfo::move(const void* src, void* dst) const
{
    if (_move != nullptr) {
        _move(src, dst);
    } else {
        memcpy(dst, src, size);
    }
}

void
Cel::ComponentInfo::destructor(const void* ptr, const size_t n) const
{
    if (_destructor != nullptr) {
        _destructor(ptr, n);
    }
}

Cel::Archetype::~Archetype()
{
    for (const auto& store : storages) {

        // If not trivially destructible, we must actually execute the
        // destructor

        store.info.destructor(store.alloc, get_total_entities());

        // Lastly, free the allocation
        ::operator delete(store.alloc, std::align_val_t{ store.info.align });
    }
}

void*
Cel::ComponentStorage::get_component(const size_t index) const
{
    return alloc + size * index;
}

void*
Cel::Archetype::get_component(const Entity entity, const ComponentId component)
{
    assert(entityToIndex.contains(entity));

    const auto index = entityToIndex.at(entity);

    return storages[componentToIndex[component]].get_component(index);
}

void
Cel::Archetype::copy_component_to_storage(const void* src,
                                          void* dst,
                                          const ComponentStorage& store)
{
    store.info.move(src, dst);
}

void
Cel::Archetype::destroy_entity(const Entity entity)
{
    assert(entityToIndex.contains(entity));

    const auto index = entityToIndex[entity];
    // Swap the last element of the array to the deleted components place
    // This keeps the array tightly packed
    const auto last = indexToEntity.size() - 1;

    for (const auto& store : storages) {
        auto position = store.get_component(index);

        store.info.destructor(position, 1);

        // Skip swapping if there's nothing to swap with
        if (index == last) {
            continue;
        }

        // Swap with the last entity
        auto lastPosition = store.get_component(last);

        copy_component_to_storage(lastPosition, position, store);

        // Once we've moved we must run the destructor for the swapped as well

        store.info.destructor(lastPosition, 1);
    }

    // Remove this entity from the map
    entityToIndex.erase(entity);
    indexToEntity.pop_back();

    // Skip the rest of we delete the last element
    if (index == last) {
        return;
    }

    // Update the mapping for the entity we're swapping
    auto swappedEntity = indexToEntity[last];

    entityToIndex[swappedEntity] = index;
    indexToEntity[index] = swappedEntity;
}

void
Cel::Archetype::register_entity(Entity entity)
{
    entityToIndex.emplace(entity, get_total_entities());
    indexToEntity.push_back(entity);
}

void
Cel::Archetype::move_component(const Entity entity,
                               const ComponentId component,
                               const void* src)
{
    const auto index = entityToIndex.at(entity);

    const auto& store = storages[componentToIndex[component]];

    const auto dst = store.get_component(index);

    copy_component_to_storage(src, dst, store);
}

size_t
Cel::Archetype::get_entity_index(const Entity entity) const
{
    return entityToIndex.at(entity);
}

Cel::Entity
Cel::Archetype::get_entity_from_index(const size_t index) const
{
    return indexToEntity[index];
}

size_t
Cel::Archetype::get_total_entities() const
{
    return indexToEntity.size();
}

const Cel::Signature&
Cel::Archetype::get_signature() const
{
    return signature;
}

Cel::ComponentStorage&
Cel::Archetype::get_storage(const ComponentId component)
{
    return storages[componentToIndex[component]];
}
