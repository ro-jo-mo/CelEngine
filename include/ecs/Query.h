#pragma once

#include "ComponentArray.h"
#include "ComponentsManager.h"
#include "IQuery.h"

#include "Types.h"
#include <algorithm>

#include <vector>
#include <iostream>
#include <iterator>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <limits>

namespace Cel {
  /**
   * @ingroup Helper types
   * @{
   */

  template<typename T>
  struct TypeStorage {
  };

  template<typename... T>
  struct TypeList {
  };

  template<typename... T>
  struct With : TypeList<T...> {
  };

  template<typename... T>
  struct Without : TypeList<T...> {
  };

  /**
   * @}
   */


  /**
   * @brief A special helper function for getting the index of "Entity" in a query
   * @param index Starts at 0, recursively searches for entity
   * @return The index of entity in the query
   */
  template<typename Component, typename... Components>
  consteval size_t GetIndexOfEntityType(const size_t index) {
    if constexpr (std::is_same_v<Component, Entity>) {
      return index;
    }
    if constexpr (sizeof...(Components) > 0) {
      return GetIndexOfEntityType<Components...>(index + 1);
    }
    return -1;
  }

  /**
   * Base form of query, no implementation
   * Always use Query<With<...>,Without<...>>
   */
  template<typename...>
  class Query;

  /**
   * @brief Standard query, requests a filtered set of entities required by a game system
   * @tparam Include Only include entities with all of these components
   * @tparam Exclude Exclude all entities with any of these components
   */
  template<typename... Include, typename... Exclude>
  class Query<With<Include...>, Without<Exclude...> >
      : public IQuery {
  public:
    explicit Query(ComponentsManager &components_manager) : manager(components_manager) {
      includedComponentArrays =
          std::make_tuple(manager.GetComponentArray<Include>()...);
      Query::UpdateQuery();
    }

    void UpdateQuery() override;

    /**
     * @brief Retrieve the queried components of this specific entity
     * @param entity Entity to retrieve
     * @return Retrieved entity components
     */
    std::tuple<Include &...> Get(Entity entity);

    /**
     * Iterator used for query iteration
     */
    class Iterator;

    Iterator begin();

    Iterator end();

  private:
    /**
     * @brief Special method to retrieve the specified component.
     * As previously stated entity ids are a special case of components, not stored by the component manager and must be fetched separately
     * @tparam Component Component to retrieve
     * @param entity Entity owning this component
     * @return Retrieved component
     */
    template<typename Component>
    auto &GetComponentOrEntity(Entity &entity);

    std::tuple<std::shared_ptr<ComponentArray<Include> >...> includedComponentArrays;
    ComponentsManager &manager;
    std::unordered_set<Entity> included;
    size_t entityTypeIndex = GetIndexOfEntityType<Include...>(0);
  };

  template<typename... Include, typename... Exclude>
  inline void
  Query<With<Include...>,
    Without<Exclude...> >::UpdateQuery() {
    auto getEntityLists = [](auto &... componentArrays) {
      return std::vector<std::unordered_map<Entity, size_t> >{
        componentArrays->GetEntityList()...
      };
    };

    auto entityArrays = std::apply(getEntityLists, includedComponentArrays);

    if (entityTypeIndex != -1) {
      entityArrays.erase(entityArrays.begin() + entityTypeIndex);
    }

    // find smallest array, use it as the base
    size_t smallestIndex = 0;
    size_t smallestSize = std::numeric_limits<size_t>::max();

    for (int i = 0; i < entityArrays.size(); ++i) {
      if (entityArrays[i].size() < smallestSize) {
        smallestSize = entityArrays[i].size();
        smallestIndex = i;
      }
    }
    // put smallest list at the front
    std::swap(entityArrays[0], entityArrays[smallestIndex]);

    included.clear();

    for (auto &[entity, _]: entityArrays[0]) {
      auto inSet = std::all_of(
        entityArrays.begin() + 1, entityArrays.end(), [entity](auto &map) {
          return map.find(entity) != map.end();
        });

      if (inSet) {
        included.insert(entity);
      }
    }

    // for each excluded list, does entity exist inside
    // if so, remove
    auto excludedComponentArrays = std::make_tuple(manager.GetComponentArray<Exclude>()...);
    auto excludedEntityArrays = std::apply(getEntityLists, excludedComponentArrays);

    for (auto iter = included.begin(); iter != included.end();) {
      auto entity = *iter;
      auto inSet = std::any_of(
        excludedEntityArrays.begin(),
        excludedEntityArrays.end(),
        [entity](auto &map) { return map.find(entity) != map.end(); });

      if (inSet) {
        iter = included.erase(iter);
      } else {
        ++iter;
      }
    }
  }


  template<typename... Include, typename... Exclude>
  std::tuple<Include &...>
  Query<With<Include...>, Without<Exclude...> >::Get(Entity entity) {
    return std::tuple<Include &...>(GetComponentOrEntity<Include>(entity)...);
  }

  template<typename... Include, typename... Exclude>
  template<typename Component>
  auto &Query<With<Include...>, Without<Exclude...> >::GetComponentOrEntity(Entity &entity) {
    if constexpr (std::is_same_v<Component, Entity>) {
      return (entity);
    }
    return std::get<std::shared_ptr<ComponentArray<Component> > >(includedComponentArrays)->GetComponent(entity);
  }

  template<typename... Include, typename... Exclude>
  class Query<With<Include...>, Without<Exclude...> >::Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;

    using Iter = std::unordered_set<Entity>::iterator;


    Iterator(Iter currentIter,
             Iter endIter,
             Query<With<Include...>,
               Without<Exclude...> > &viewRef)
      : current(currentIter)
        , end(endIter)
        , view(viewRef) {
    };

    bool operator!=(const Iterator &other) const {
      return current != other.current;
    }

    Iterator &operator++() {
      ++current;
      return *this;
    }

    std::tuple<Include &...> operator*() {
      ref = *current;
      return view.Get(ref);
    }

  private:
    Iter current;
    Iter end;
    Entity ref{0};
    Query<With<Include...>, Without<Exclude...> > &view;
  };

  template<typename... Include, typename... Exclude>
  Query<With<Include...>, Without<Exclude...> >::Iterator Query<With<Include...>, Without<Exclude...> >::begin() {
    return Iterator(included.begin(), included.end(), *this);
  }

  template<typename... Include, typename... Exclude>
  Query<With<Include...>, Without<Exclude...> >::Iterator Query<With<Include...>, Without<Exclude...> >::end() {
    return Iterator(included.end(), included.end(), *this);
  }

  // Additional specialisation for convenience, when there is no without filter
  template<typename... Include>
  class Query<With<Include...> > final : public Query<With<Include...>, Without<> > {
  public:
    explicit Query(ComponentsManager &components_manager) : Query<With<Include...>, Without<> >(components_manager) {
    }
  };
}


