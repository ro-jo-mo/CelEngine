#pragma once
#include "Types.h"
#include "core/TypeId.h"

#include <array>
#include <core/Config.h>
#include <unordered_map>

namespace Cel {

// Archetype manager?

struct ComponentInfo
{
    size_t size = 0;
    size_t align = 0;

    void (*move)(const void* src, const void* dst) = nullptr;

    void (*destructor)(const void* ptr, size_t n) = nullptr;
};

struct ComponentStorage
{
    const ComponentInfo& info;

    // Technically duplicate info but it means we don't need to load the
    // component info for accesses

    // The size of an individual element in the allocation
    size_t size;
    // Alignment of the allocation, needed to free it
    size_t align;

    char* alloc;

    [[nodiscard]] void* get_component(size_t index) const;
};

class Archetype
{
  public:
    Archetype(const std::array<ComponentInfo, MAX_COMPONENTS>& componentInfos,
              const Signature& signature);

    Archetype(Archetype&) = delete;

    ~Archetype();

    // Registers this entity as a member of this archetype, but does not set the
    // components
    void register_entity(Entity entity);

    void* get_component(Entity entity, ComponentId component);

    void move_component(Entity, ComponentId component, void* src);

    void delete_entity(Entity entity);

    [[nodiscard]] size_t get_entity_index(Entity entity) const;
    [[nodiscard]] Entity get_entity_from_index(size_t index) const;
    [[nodiscard]] size_t get_total_entities() const;
    [[nodiscard]] const Signature& get_signature() const;
    [[nodiscard]] ComponentStorage& get_storage(ComponentId component);

  private:
    // Copies the data from src to dst (a pointer inside the component storage)
    // Does not destroy the moved value
    static void copy_component_to_storage(const void* src,
                                          void* dst,
                                          const ComponentStorage& store);

    std::vector<ComponentStorage> storages{};

    // Maps component id to an index in the storage array
    std::array<uint16_t, MAX_COMPONENTS> componentToIndex{};

    // Maps an entity to an index in the storage allocations
    std::unordered_map<Entity, size_t> entityToIndex;
    // Maps an index in the storage allocations to an entity
    // Also used to get the total number of entities in this archetype
    std::vector<Entity> indexToEntity;

    Signature signature;
};

}