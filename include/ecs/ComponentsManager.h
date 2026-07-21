#pragma once

#include "ComponentArray.h"
#include "Types.h"
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace Cel {
/**
 * @brief The overarching components storage for the entire engine. Stores every
 * component that currently exists.
 */
class ComponentsManager
{
  public:
    /**
     * @brief Registers a new component type.
     * @tparam T The component type to register
     */
    template<typename T>
    void register_component();

    /**
     * @brief Retrieves a compoent of type T owned by the entity
     * @tparam T The type of the component
     * @param entity Entity owning component T
     * @return The component
     */
    template<typename T>
    T& get_component(Entity entity);

    /**
     * @brief Add a component of type T to the entity
     * @tparam T Component type
     * @param entity Entity to add to
     * @param component Component data to add
     */
    template<typename T>
    void add_component(Entity entity, T component);

    template<typename T>
    bool has_component(Entity entity);
    /**
     * @brief Remove a component of type T from entity
     * @tparam T Component type
     * @param entity Entity to remove from
     */
    template<typename T>
    void remove_component(Entity entity);

    /**
     * @brief Get all components of this type. Required by queries.
     * @tparam T Component type
     * @return the component array
     */
    template<typename T>
    std::shared_ptr<ComponentArray<T>> get_component_array();

    /**
     * @brief Completely remove all components owned by entity
     * @param entity Entity to destroy
     */
    void destroy_entity(Entity entity);

  private:
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>>
        componentArrays;
};

template<typename T>
inline void
ComponentsManager::register_component()
{
    if (componentArrays.contains(typeid(T))) {
        return;
    }
    componentArrays[typeid(T)] = std::make_shared<ComponentArray<T>>();
}
template<typename T>
T&
ComponentsManager::get_component(Entity entity)
{
    return get_component_array<T>()->get_component(entity);
}

template<typename T>
bool
ComponentsManager::has_component(Entity entity)
{
    return get_component_array<T>()->has_component(entity);
}

template<typename T>
inline void
ComponentsManager::add_component(Entity entity, T component)
{
    get_component_array<T>()->add_component(entity, component);
}

template<typename T>
inline void
ComponentsManager::remove_component(Entity entity)
{
    get_component_array<T>()->remove_component(entity);
}

template<typename T>
inline std::shared_ptr<ComponentArray<T>>
ComponentsManager::get_component_array()
{
    register_component<T>();
    return std::static_pointer_cast<ComponentArray<T>>(
        componentArrays[typeid(T)]);
}
}
