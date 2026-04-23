#pragma once

#include "ComponentArray.h"
#include "Types.h"
#include <array>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace Cel {
  /**
   * @brief The overarching components storage for the entire engine. Stores every component that currently exists.
   */
  class ComponentsManager {
  public:
    /**
     * @brief Registers a new component type.
     * @tparam T The component type to register
     */
    template<typename T>
    void RegisterComponent();

    /**
     * @brief Retrieves a compoent of type T owned by the entity
     * @tparam T The type of the component
     * @param entity Entity owning component T
     * @return The component
     */
    template<typename T>
    T &GetComponent(Entity entity);

    /**
     * @brief Add a component of type T to the entity
     * @tparam T Component type
     * @param entity Entity to add to
     * @param component Component data to add
     */
    template<typename T>
    void AddComponent(Entity entity, T component);

    /**
     * @brief Remove a component of type T from entity
     * @tparam T Component type
     * @param entity Entity to remove from
     */
    template<typename T>
    void RemoveComponent(Entity entity);

    /**
     * @brief Get all components of this type. Require by queries.
     * @tparam T Component type
     * @return the component array
     */
    template<typename T>
    std::shared_ptr<ComponentArray<T> > GetComponentArray();

    /**
     * @brief Completely remove all components owned by entity
     * @param entity Entity to destroy
     */
    void DestroyEntity(Entity entity);

  private:
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray> > componentArrays;
  };

  template<typename T>
  inline void
  ComponentsManager::RegisterComponent() {
    if (componentArrays.contains(typeid(T))) {
      return;
    }
    componentArrays[typeid(T)] = std::make_shared<ComponentArray<T> >();
  }

  template<typename T>
  inline void
  ComponentsManager::AddComponent(Entity entity, T component) {
    GetComponentArray<T>()->AddComponent(entity, component);
  }

  template<typename T>
  inline void
  ComponentsManager::RemoveComponent(Entity entity) {
    GetComponentArray<T>()->RemoveComponent(entity);
  }

  template<typename T>
  inline std::shared_ptr<ComponentArray<T> >
  ComponentsManager::GetComponentArray() {
    RegisterComponent<T>();
    return std::static_pointer_cast<ComponentArray<T> >(
      componentArrays[typeid(T)]);
  }
}
