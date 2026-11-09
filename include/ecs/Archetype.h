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

class Archetype
{
  public:
    Archetype(const std::array<ComponentInfo, MAX_COMPONENTS>& componentInfos,
              const Signature& signature);

    ~Archetype();

    void move_entity(Entity entity);

    void* get_component(Entity entity, ComponentId component);

    void delete_entity(Entity entity);

  private:
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
    };

    static void* get_component_from_entity_index(size_t index,
                                                 const ComponentStorage& store);

    std::vector<ComponentStorage> storages{};

    // Maps component id to an index in the storage array
    std::array<uint16_t, MAX_COMPONENTS> componentToIndex;

    // Maps an entity to an index in the storage allocations
    std::unordered_map<Entity, size_t> entityToIndex;
    // Maps an index in the storage allocations to an entity
    std::unordered_map<size_t, Entity> indexToEntity;

    size_t totalEntities = 0;

    const std::array<ComponentInfo, MAX_COMPONENTS>& componentInfos;
};

}