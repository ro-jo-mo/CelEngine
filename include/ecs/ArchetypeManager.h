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

    template<typename T>
    T* get_component_for_entity(Entity entity);

    void* get_component_for_entity(Entity entity, ComponentId component);

    [[nodiscard]] const Signature& get_entity_signature(Entity entity) const;

    [[nodiscard]] const ComponentInfo& get_component_info(
        ComponentId component) const;

    template<typename T>
    bool has_component(Entity entity) const;

    void destroy_entity(Entity entity);

    struct UpdateInfo
    {
        Signature signature;
        std::array<void*, MAX_COMPONENTS> datas{};
    };

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

    // Skip already registered components
    if (info.size != 0) {
        return;
    }

    info.size = sizeof(T);
    info.align = alignof(T);

    // No need to set the destructor / move constructor if they're both trivial
    if (!std::is_trivially_copyable<T>()) {
        info._move = [](const void* src, const void* dst) {
            std::construct_at(static_cast<T*>(dst),
                              std::move(*static_cast<T*>(src)));
        };
    }

    if (!std::is_trivially_destructible<T>()) {
        info._destructor = [](const void* ptr, size_t n) {
            std::destroy_n(static_cast<const T*>(ptr), n);
        };
    }
}

template<typename T>
bool
ArchetypeManager::has_component(const Entity entity) const
{
    return get_entity_signature(entity).test(component_id<T>());
}

template<typename T>
T*
ArchetypeManager::get_component_for_entity(const Entity entity)
{
    return static_cast<T*>(get_component_for_entity(entity, component_id<T>()));
}

}
