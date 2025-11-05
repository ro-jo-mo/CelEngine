#pragma once

#include "Types.h"
#include <array>
#include <unordered_map>

namespace Cel {
  class IComponentArray {
  public:
    virtual void DestroyEntity(Entity entity) = 0;
  };

  template<typename T>
  class ComponentArray : public IComponentArray {
  public:
    void DestroyEntity(const Entity entity) override {
      if (entityToComponent.contains(entity)) {
        RemoveComponent(entity);
      }
    }

    void AddComponent(Entity entity, T component);

    void RemoveComponent(Entity entity);

    T &GetComponent(Entity entity);

    const std::unordered_map<Entity, size_t> &GetEntityList();

  private:
    std::array<T, MAX_ENTITIES> components;
    std::unordered_map<Entity, size_t> entityToComponent;
    std::unordered_map<size_t, Entity> componentToEntity;
    Entity totalComponents;
  };

  template<typename T>
  void
  ComponentArray<T>::AddComponent(const Entity entity, T component) {
    components[totalComponents] = component;
    entityToComponent[entity] = totalComponents;
    componentToEntity[totalComponents] = entity;
    totalComponents++;
  }

  template<typename T>
  void
  ComponentArray<T>::RemoveComponent(const Entity entity) {
    auto index = entityToComponent[entity];

    auto last = --totalComponents;
    components[index] = components[last];

    componentToEntity.erase(index);
    entityToComponent.erase(entity);
  }

  template<typename T>
  T &
  ComponentArray<T>::GetComponent(const Entity entity) {
    auto index = entityToComponent[entity];
    return components[index];
  }

  template<typename T>
  const std::unordered_map<Entity, size_t> &
  ComponentArray<T>::GetEntityList() {
    return entityToComponent;
  }
}
