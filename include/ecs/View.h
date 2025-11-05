#pragma once

#include "ComponentArray.h"
#include "ComponentsManager.h"
#include "IView.h"
#include "SystemManager.h"
#include "Types.h"
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <tuple>
#include <unordered_set>


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


  template<typename, typename>
  class View;

  template<typename... Include, typename... Exclude>
  class View<With<Include...>, Without<Exclude...> > final
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
    explicit View() = default;

    std::tuple<std::shared_ptr<ComponentArray<Include> >...>
    includedComponentArrays;
    std::shared_ptr<ComponentsManager> manager;
    std::unordered_set<Entity> included;
  };


  template<typename... Include, typename... Exclude>
  auto View<With<Include...>, Without<Exclude...> >::Create() {
    // Add to queue
    // When ready initialise, set shared pointer to component manager etc
    // TODO: If this view already exists for another system, simply return that view instead of duplicating
    auto view = std::make_shared<View<With<Include...>, Without<Exclude...> > >();
    SystemManager::Queue(view);
    return view;
  }

  template<typename... Include, typename... Exclude>
  void View<With<Include...>, Without<Exclude...> >::Initialise(std::shared_ptr<ComponentsManager> mngr) {
    this->manager = mngr;

    includedComponentArrays =
        std::make_tuple(manager->GetComponentArray<Include>()...);

    UpdateView();
  }


  template<typename... Include, typename... Exclude>
  inline void
  View<With<Include...>,
    Without<Exclude...> >::UpdateView() {
    auto getEntityLists = [](auto &&componentArrays) {
      return std::array{componentArrays->GetEntityList()};
    };

    auto entityArrays = std::apply(getEntityLists, includedComponentArrays);

    // find smallest array, use it as the base
    size_t smallestIndex = 0;
    size_t smallestSize = 99999;

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
    auto excludedEntityArrays =
        std::invoke(getEntityLists, manager->GetComponentArray<Exclude>()...);

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
  inline std::tuple<Include &...>
  View<With<Include...>, Without<Exclude...> >::Get(
    Entity entity) {
    return std::make_tuple(
      std::get<Include>(includedComponentArrays)->GetComponent(entity)...);
  }


  template<typename... Include, typename... Exclude>
  class View<With<Include...>, Without<Exclude...> >::Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;

    using Iter = std::unordered_set<Entity>::iterator;


    Iterator(Iter currentIter,
             Iter endIter,
             View<With<Include...>,
               Without<Exclude...> > &viewRef)
      : current(currentIter)
        , end(endIter)
        , view(viewRef) {
    };

    bool operator!=(const Iterator &other) const {
      return current != other.current;
    }

    void operator++() { ++current; }

    std::tuple<Include &...> operator*() {
      Entity entity = *current;
      return view.Get(entity);
    }

  private:
    Iter current;
    Iter end;
    View<With<Include...>, Without<Exclude...> > &
    view;
  };

  template<typename... Include, typename... Exclude>
  View<With<Include...>, Without<Exclude...> >::Iterator View<With<Include...>, Without<Exclude
    ...> >::begin() {
    return Iterator(included.begin(), included.end(), &this);
  }

  template<typename... Include, typename... Exclude>
  View<With<Include...>, Without<Exclude...> >::Iterator View<With<Include...>, Without<Exclude...> >::end() {
    return Iterator(included.end(), included.end(), &this);
  }
}


