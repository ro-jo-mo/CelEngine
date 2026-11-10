#pragma once

#include "ArchetypeManager.h"
#include "ComponentArray.h"
#include "ComponentsManager.h"
#include "IQuery.h"

#include "Types.h"
#include "core/Optional.h"

#include <algorithm>

#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Cel {

/**
 * @ingroup Helper types
 * @{
 */

template<typename... T>
struct TypeList
{};

template<typename... T>
struct Get : TypeList<T...>
{};

template<typename... T>
struct With : TypeList<T...>
{};

template<typename... T>
struct Without : TypeList<T...>
{};

/**
 * @}
 */

/**
 * Base form of query, no implementation
 * Always use Query<With<...>,Without<...>>
 */
template<typename...>
class Query;

/**
 * @brief Standard query, requests a filtered set of entities required by a
 game
 * system
 * @tparam Requires Only include entities with all of these components
 * @tparam Rejects Exclude all entities with any of these components
 */
template<typename... Retrieves, typename... Requires, typename... Rejects>
class Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>
    : public IQuery
{
    // Account for entity retrieval and optionals
    // If T == Entity "Entity" Else If IsOptional T::type* Else T&
    template<typename T>
    using ReturnT = std::conditional_t<
        std::is_same_v<Entity, T>,
        Entity,
        std::conditional_t<IsOptional<T>::value, typename T::type*, T&>>;

    // Type returned from get + iter
    using ReturnTuple = std::tuple<ReturnT<Retrieves>...>;

  public:
    explicit Query(ArchetypeManager& archetypeManager);

    ReturnTuple get(Entity entity);

    bool has(Entity entity) const;

    size_t size() const;

    struct Iterator
    {
        bool operator!=(const Iterator& other) const;

        bool operator==(const Iterator& other) const;

        Iterator& operator++();

        ReturnTuple operator*();
    };

    Iterator begin();

    Iterator end();

  private:
    template<typename T>
    ReturnT<T> get_return_value(Entity entity);

    // We must store the manager for "get"
    ArchetypeManager& archetypeManager;
};

template<typename... Retrieves, typename... Requires, typename... Rejects>
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::Query(
    ArchetypeManager& archetypeManager)
    : archetypeManager(archetypeManager)
{
    // Set signatures

    auto mark_sig = []<typename T>(Signature& sig) {
        sig.set(component_id<T>(), true);
    };

    // Version that ignores optional types
    auto mark_sig_ignore_optional = []<typename T>(Signature& sig) {
        if constexpr (!IsOptional<T>::value) {
            sig.set(component_id<T>(), true);
        }
    };

    // If type is optional, get type held
    auto mark_sig_strip_optional = []<typename T>(Signature& sig) {
        if constexpr (IsOptional<T>::value) {
            sig.set(component_id<typename T::type>(), true);
        } else {
            sig.set(component_id<T>(), true);
        }
    };

    // Components that this query requires an entity has
    (void(mark_sig_ignore_optional.template operator()<Retrieves>(required)),
     ...);
    (void(mark_sig.template operator()<Requires>(required)), ...);

    // Components this query will retrieve, including optionals
    (void(mark_sig_strip_optional.template operator()<Retrieves>(retrieve)),
     ...);

    // Components this query will reject
    (void(mark_sig.template operator()<Rejects>(reject)), ...);
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::ReturnTuple
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::get(
    const Entity entity)
{
    // Get uses a different routing to iteration

    // Ensure entity is actually a match for this query
    assert(
        has(entity) &&
        "Entity retrieved through Query::get(entity) must exist in the query");

    return ReturnTuple(get_return_value<Retrieves>(entity)...);
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
template<typename T>
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::
    template ReturnT<T>
    Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::
        get_return_value(Entity entity)
{
    if constexpr (std::is_same_v<Entity, T>) {
        return entity;
    } else {
        // Reinterpret cast should handle ref vs pointer for optional
        return reinterpret_cast<ReturnT<T>>(
            archetypeManager.get_component_for_entity(entity,
                                                      component_id<T>()));
    }
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
bool
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::has(
    const Entity entity) const
{
    return (archetypeManager.get_entity_signature(entity) & required) ==
           required;
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
size_t
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::size() const
{
    size_t total = 0;

    for (const auto& match : matches) {
        total += match.archetype.get_total_entities();
    }

    return total;
}

}
