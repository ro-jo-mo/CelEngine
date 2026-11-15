#pragma once

#include "ArchetypeManager.h"
#include "IQuery.h"

#include "Types.h"
#include "core/Optional.h"

#include <algorithm>
#include <cassert>
#include <limits>

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

namespace Detail {
// What a query hands back for each Get<...> entry:
// Entity -> Entity, Optional<T> -> T*, T -> T&
// (specialisations rather than std::conditional_t, so T::type is only
// formed for Optional<T>)
template<typename T>
struct QueryReturn
{
    using type = T&;
};

template<>
struct QueryReturn<Entity>
{
    using type = Entity;
};

template<typename T>
struct QueryReturn<Optional<T>>
{
    using type = T*;
};

// Sentinel stored in retrieveOrder for Entity, which isn't a component
constexpr ComponentId NO_COMPONENT = std::numeric_limits<ComponentId>::max();

template<typename T>
constexpr ComponentId
retrieve_id()
{
    if constexpr (is_entity<T>) {
        return NO_COMPONENT;
    } else if constexpr (is_optional<T>) {
        return component_id<typename T::type>();
    } else {
        return component_id<T>();
    }
}
}

/**
 * @}
 */

template<typename...>
class BasicQuery;

/**
 * @brief Standard query, requests a filtered set of entities required by a
 game
 * system
 * @tparam Retrieves Get these components from entities that match the required
 filters
 * @tparam Requires Only include entities with all of these components
 * @tparam Rejects Exclude all entities with any of these components
 */
template<typename... Retrieves, typename... Requires, typename... Rejects>
class BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>
    : public IQuery
{
    static_assert(sizeof...(Retrieves) <= MAX_QUERY_RETRIEVE,
                  "Query retrieves more components than MAX_QUERY_RETRIEVE");

    template<typename T>
    using ReturnT = typename Detail::QueryReturn<T>::type;

    // Type returned from get + iter
    using ReturnTuple = std::tuple<ReturnT<Retrieves>...>;

  public:
    explicit BasicQuery(ArchetypeManager& archetypeManager);

    ReturnTuple get(Entity entity);

    bool has(Entity entity) const;

    size_t size() const;

    struct Iterator
    {
        bool operator!=(const Iterator& other) const
        {
            return !(*this == other);
        }

        bool operator==(const Iterator& other) const
        {
            return matchIndex == other.matchIndex &&
                   entityIndex == other.entityIndex;
        }

        Iterator& operator++()
        {
            entityIndex++;
            skip_empty();

            return *this;
        }

        ReturnTuple operator*()
        {
            auto& match = query.matches[matchIndex];

            return [&]<size_t... Is>(std::index_sequence<Is...>) {
                return ReturnTuple(get_component<Retrieves, Is>(match)...);
            }(std::index_sequence_for<Retrieves...>{});
        }

        template<typename T, size_t Index>
        ReturnT<T> get_component(QueryMatch& match)
        {
            if constexpr (is_entity<T>) {
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

        // Advance past archetypes that currently hold no entities
        void skip_empty()
        {
            while (
                matchIndex < query.matches.size() &&
                entityIndex >=
                    query.matches[matchIndex].archetype.get_total_entities()) {
                entityIndex = 0;
                matchIndex++;
            }
        }

        size_t matchIndex;
        size_t entityIndex;

        BasicQuery& query;
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
BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::
    BasicQuery(ArchetypeManager& archetypeManager)
    : archetypeManager(archetypeManager)
{
    // Set retrieve data
    totalRetrieved = sizeof...(Retrieves);
    retrieveOrder.fill(Detail::NO_COMPONENT);

    [&]<size_t... Is>(std::index_sequence<Is...>) {
        ((retrieveOrder[Is] = Detail::retrieve_id<Retrieves>()), ...);
    }(std::index_sequence_for<Retrieves...>{});

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
typename BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::
    ReturnTuple
    BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::get(
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
typename BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::
    template ReturnT<T>
    BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::
        get_return_value(Entity entity)
{
    if constexpr (is_entity<T>) {
        return entity;
    } else if constexpr (is_optional<T>) {
        return static_cast<ReturnT<T>>(
            archetypeManager.get_component_for_entity(
                entity, component_id<typename T::type>()));
    } else {
        return *static_cast<std::remove_reference_t<ReturnT<T>>*>(
            archetypeManager.get_component_for_entity(entity,
                                                      component_id<T>()));
    }
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
bool
BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::has(
    const Entity entity) const
{
    const auto& signature = archetypeManager.get_entity_signature(entity);

    return (signature & required) == required && (signature & reject).none();
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
size_t
BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::size()
    const
{
    size_t total = 0;

    for (const auto& match : matches) {
        total += match.archetype.get_total_entities();
    }

    return total;
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
typename BasicQuery<Get<Retrieves...>,
                    With<Requires...>,
                    Without<Rejects...>>::Iterator
BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::begin()
{
    Iterator it{ .matchIndex = 0, .entityIndex = 0, .query = *this };
    it.skip_empty();
    return it;
}

template<typename... Retrieves, typename... Requires, typename... Rejects>
typename BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::
    Iterator
    BasicQuery<Get<Retrieves...>, With<Requires...>, Without<Rejects...>>::end()
{
    return { .matchIndex = matches.size(), .entityIndex = 0, .query = *this };
}

template<template<typename...> class Wrapper, typename... Ts>
struct FindOrEmptyImpl;

// Base case, no wrapper
template<template<typename...> class Wrapper>
struct FindOrEmptyImpl<Wrapper>
{
    using type = Wrapper<>;
};

// Case we find wrapper
template<template<typename...> class Wrapper, typename... Ts, typename... Rs>
struct FindOrEmptyImpl<Wrapper, Wrapper<Ts...>, Rs...>
{
    using type = Wrapper<Ts...>;
};

// Recursive case for no match
template<template<typename...> class Wrapper, typename T, typename... Ts>
struct FindOrEmptyImpl<Wrapper, T, Ts...>
{
    using type = typename FindOrEmptyImpl<Wrapper, Ts...>::type;
};

// e.g. FindOrEmtpy<With,Get<...>,With<A,B,...>,Without<...>>
// deduces to With<A,B,...>
template<template<typename...> class T, typename... Ts>
using FindOrEmpty = typename FindOrEmptyImpl<T, Ts...>::type;

// Is T a specialisation of Wrapper<...>
template<template<typename...> class Wrapper, typename T>
constexpr bool is_specialisation_of = false;

template<template<typename...> class Wrapper, typename... Ts>
constexpr bool is_specialisation_of<Wrapper, Wrapper<Ts...>> = true;

// How many of Ts are Wrapper<...>
template<template<typename...> class Wrapper, typename... Ts>
constexpr size_t count_specialisations_of =
    (size_t{ 0 } + ... + (is_specialisation_of<Wrapper, Ts> ? 1 : 0));

template<typename... Ts>
struct ValidateQueryArgs
{
    static_assert(count_specialisations_of<Get, Ts...> == 1,
                  "Query requires exactly one Get<...>");
    static_assert(count_specialisations_of<With, Ts...> <= 1,
                  "Query accepts at most one With<...>");
    static_assert(count_specialisations_of<Without, Ts...> <= 1,
                  "Query accepts at most one Without<...>");
    static_assert(
        ((is_specialisation_of<Get, Ts> || is_specialisation_of<With, Ts> ||
          is_specialisation_of<Without, Ts>) &&
         ...),
        "Query arguments must be Get<...>, With<...> or Without<...>");

    using type = BasicQuery<FindOrEmpty<Get, Ts...>,
                            FindOrEmpty<With, Ts...>,
                            FindOrEmpty<Without, Ts...>>;
};

/**
 * @brief Query<Get<...>, With<...>, Without<...>>
 * Create a query that retrieves components in Get<...>
 * Returns only entities that have both Get<...> and With<...>, and do not
 * have Without<...>
 *
 * Use Cel::Get<Cel::Optional<MyComponent>> to optionally retrieve a component
 */
template<typename... Ts>
using Query = typename ValidateQueryArgs<Ts...>::type;

}
