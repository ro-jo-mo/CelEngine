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
    void get_components(Entity entity);

  private:
    struct EntityData
    {
        Archetype& archetype;
        size_t index;
    };

    template<typename T>
    void register_component();

    std::unordered_map<Signature, Archetype> archetypes;
    std::vector<EntityData> entityDatas;

    std::array<ComponentInfo, MAX_COMPONENTS> componentInfos{};
};

template<typename T>
void
ArchetypeManager::register_component()
{
    auto& info = componentInfos[component_id<T>()];

    info.size = sizeof(T);

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
