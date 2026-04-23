#pragma once

#include "Types.h"
#include <array>
#include <unordered_map>

namespace Cel {
  /**
   * @brief A dummy interface so component arrays can be stored in a list of the same type.
   */
  class IComponentArray {
  public:
    /**
     * The only command we require to be run on every component array.
     * Hence why it is located in the interface
     * @param entity Entity to destroy
     */
    virtual void DestroyEntity(Entity entity) = 0;
  };

  /**
   * @brief A container storing components, and mappings of the entity that owns it.
   * @tparam T The component we're storing.
   */
  template<typename T>
  class ComponentArray : public IComponentArray {
  public:
    void DestroyEntity(const Entity entity) override {
      if (entityToComponent.contains(entity)) {
        RemoveComponent(entity);
      }
    }

    /**
     * @brief Add a new component to this entity
     * @param entity The entity owning this component
     * @param component The component data
     */
    void AddComponent(Entity entity, T component);

    /**
     * @brief Remove a component from this entity
     * @param entity The entity that owns the component.
     */
    void RemoveComponent(Entity entity);

    /**
     * @brief Return the component of type T owned by this entity
     * @param entity The entity owning this component
     * @return The owned component
     */
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
