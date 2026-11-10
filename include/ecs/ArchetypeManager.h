#pragma once

#include "Archetype.h"
#include "Types.h"
#include "core/TypeId.h"

#include <memory>
#include <unordered_map>

namespace Cel {

class ArchetypeManager
{
  public:
    ArchetypeManager();

    void* get_component_for_entity(Entity entity, ComponentId component);

    const Signature& get_entity_signature(Entity entity) const;

  private:
    struct EntityData
    {
        Archetype* archetype;
        size_t index;
    };

    // Set the component data for this component
    template<typename T>
    void register_component();

    // Transfer an entity from one archetype to another
    void move_entity(
        Entity entity,
        const Signature& previousSig,
        const Signature& newSig,
        const std::unordered_map<ComponentId, void*>& addedComponents);

    // Set the entity data for this entity
    void update_entity_data(Entity entity, const Signature& signature);

    // Storage of archetypes. We don't actually access them here, just store.
    std::vector<std::unique_ptr<Archetype>> archetypes;
    // Retrieve an archetype based on its signature
    std::unordered_map<Signature, Archetype*> sigToArchetype;
    // Stores data about the components an entity has and where it's stored
    std::array<EntityData, MAX_ENTITIES> entityData{};
    // Stores details about the sizing, destructor and constructor of a
    // component
    std::array<ComponentInfo, MAX_COMPONENTS> componentInfos{};
};

template<typename T>
void
ArchetypeManager::register_component()
{
    auto& info = componentInfos[component_id<T>()];

    info.size = sizeof(T);
    info.align = alignof(T);

    // No need to set the destructor / move constructor if they're both trivial
    if (!std::is_trivially_copyable<T>()) {
        info.move = [](const void* src, const void* dst) {
            std::construct_at(static_cast<T*>(dst),
                              std::move(*static_cast<T*>(src)));
        };
    }

    if (!std::is_trivially_destructible<T>()) {
        info.destructor = [](const void* ptr, size_t n) {
            std::destroy_n(static_cast<const T*>(ptr), n);
        };
    }
}

}
