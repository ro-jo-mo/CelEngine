#pragma once

#include "ArchetypeManager.h"
#include "IQuery.h"

#include "Types.h"
#include "core/Optional.h"

#include <algorithm>

#include <tuple>
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

template<typename T>
constexpr bool is_entity = std::is_same_v<T, Entity>;

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
        is_entity<T>,
        Entity,
        std::conditional_t<is_optional<T>, typename T::type*, T&>>;

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

        template<typename T, size_t Index>
        ReturnT<T> get_component(QueryMatch& match);

        size_t matchIndex;
        size_t entityIndex;

        Query& query;
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
    // Set retrieve data
    constexpr size_t SIZE = sizeof...(Retrieves);

    totalRetrieved = SIZE;

    auto set_retrieve = [&]<typename T, typename... Ts>(
                            const size_t index, auto&& _recurse) -> void {
        if (index == SIZE) {
            return;
        }
        if (is_entity<T>) {

        } else if (is_optional<T>) {
            retrieveOrder[index] = component_id<typename T::type>();
        } else {
            retrieveOrder[index] = component_id<T>();
        }

        _recurse(index + 1, _recurse);
    };

    set_retrieve.template operator()<Retrieves...>(0, set_retrieve);

    // Set signatures

    auto mark_sig = []<typename T>(Signature& sig) {
        sig.set(component_id<T>(), true);
    };

    // Version that ignores optional types
    auto mark_sig_ignore_optional_entity = []<typename T>(Signature& sig) {
        if constexpr (!is_entity<T> && !is_optional<T>) {
            sig.set(component_id<T>(), true);
        }
    };

    // If type is optional, get type held
    auto mark_sig_strip_optional_entity = []<typename T>(Signature& sig) {
        if constexpr (is_entity<T>) {
        } else if constexpr (is_optional<T>) {
            sig.set(component_id<typename T::type>(), true);
        } else {
            sig.set(component_id<T>(), true);
        }
    };

    // Components that this query requires an entity has
    (void(mark_sig_ignore_optional_entity.template operator()<Retrieves>(
         required)),
     ...);
    (void(mark_sig.template operator()<Requires>(required)), ...);

    // Components this query will retrieve, including optionals
    (void(mark_sig_strip_optional_entity.template operator()<Retrieves>(
         retrieve)),
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
    if constexpr (is_entity<T>) {
        return entity;
    } else if constexpr (is_optional<T>) {
        return static_cast<ReturnT<T>>(
            archetypeManager.get_component_for_entity(entity,
                                                      component_id<T>()));
    } else {
        return *static_cast<std::remove_reference_t<ReturnT<T>>*>(
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

template<typename... Retrieves, typename... Requires, typename... Rejects>
bool
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::Iterator::
operator!=(const Iterator& other) const
{
    return !(this == other);
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
bool
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::Iterator::
operator==(const Iterator& other) const
{
    return matchIndex == other.matchIndex && entityIndex == other.entityIndex;
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::Iterator
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::begin()
{
    return { .matchIndex = 0, .entityIndex = 0, .query = *this };
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::Iterator
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::end()
{
    if (matches.empty()) {
        return begin();
    }

    return { .matchIndex = matches.size(), .entityIndex = 0, .query = this };
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::Iterator&
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::Iterator::
operator++()
{
    entityIndex++;

    while (entityIndex ==
           query.matches[matchIndex].archetype.get_total_entities()) {

        entityIndex = 0;
        matchIndex++;

        if (matchIndex == query.matches.size()) {
            break;
        }
    }

    return *this;
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::ReturnTuple
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::Iterator::
operator*()
{
    auto& match = query.matches[matchIndex];

    return ReturnTuple(
        get_component<Retrieves, std::index_sequence_for<Retrieves>>(match)...);
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
template<typename T, size_t Index>
Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::
    template ReturnT<T>
    Query<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::Iterator::
        get_component(QueryMatch& match)
{
    if constexpr (std::is_same_v<T, Entity>) {
        return match.archetype.get_entity_from_index(entityIndex);
    } else if constexpr (is_optional<T>) {

        if (match.storages[Index] == nullptr) {
            return nullptr;
        }

        return static_cast<ReturnT<T>>(
            match.storages[Index]->get_component(entityIndex));

    } else {
        return *static_cast<std::remove_reference_t<ReturnT<T>>*>(
            match.storages[Index]->get_component(entityIndex));
    }
}

}
