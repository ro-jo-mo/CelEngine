#pragma once

#include "ComponentArray.h"
#include "ComponentsManager.h"
#include "IView.h"
#include "SystemManager.h"
#include "Types.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <tuple>
#include <unordered_set>
#include <limits>

namespace Cel {
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


  template<typename, typename>
  class Query;

  template<typename... Include, typename... Exclude>
  class Query<With<Include...>, Without<Exclude...> > final
      : public IView {
  public:
    static auto Create();

    void Initialise(std::shared_ptr<ComponentsManager> mngr) override;

    void UpdateView() override;

    std::tuple<Include &...> Get(Entity entity);

    class Iterator;

    Iterator begin();

    Iterator end();

  private:
    explicit Query() = default;

    template<typename Component>
    auto &GetComponentOrEntity(Entity &entity);

    std::tuple<std::shared_ptr<ComponentArray<Include> >...> includedComponentArrays;
    std::shared_ptr<ComponentsManager> manager;
    std::unordered_set<Entity> included;
    size_t entityTypeIndex = GetIndexOfEntityType<Include...>(0);
  };


  template<typename... Include, typename... Exclude>
  auto Query<With<Include...>, Without<Exclude...> >::Create() {
    auto query = std::shared_ptr<Query<With<Include...>, Without<Exclude...> > >(
      new Query<With<Include...>, Without<Exclude...> >());
    SystemManager::Queue(query);
    return query;
  }

  template<typename... Include, typename... Exclude>
  void Query<With<Include...>, Without<Exclude...> >::Initialise(std::shared_ptr<ComponentsManager> mngr) {
    this->manager = mngr;

    includedComponentArrays =
        std::make_tuple(manager->GetComponentArray<Include>()...);

    UpdateView();
  }


  template<typename... Include, typename... Exclude>
  inline void
  Query<With<Include...>,
    Without<Exclude...> >::UpdateView() {
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
    auto excludedComponentArrays = std::make_tuple(manager->GetComponentArray<Exclude>()...);
    auto excludedEntityArrays = std::apply(getEntityLists, excludedComponentArrays);

    for (auto &entity: included) {
      auto inSet = std::any_of(
        excludedEntityArrays.begin(),
        excludedEntityArrays.end(),
        [entity](auto &map) { return map.find(entity) != map.end(); });

      if (inSet) {
        included.erase(entity);
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
    Entity ref;
    Query<With<Include...>, Without<Exclude...> > &view;
  };

  template<typename... Include, typename... Exclude>
  Query<With<Include...>, Without<Exclude...> >::Iterator Query<With<Include...>, Without<Exclude
    ...> >::begin() {
    return Iterator(included.begin(), included.end(), *this);
  }

  template<typename... Include, typename... Exclude>
  Query<With<Include...>, Without<Exclude...> >::Iterator Query<With<Include...>, Without<Exclude...> >::end() {
    return Iterator(included.end(), included.end(), *this);
  }
}


