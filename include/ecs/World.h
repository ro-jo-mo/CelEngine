#pragma once

#include "ComponentsManager.h"
#include "EntityManager.h"
#include "Helpers.h"
#include "Types.h"
#include "core/Transform.h"
#include <vector>

namespace Cel {
/**
 * @brief Manager of the game world state
 * Responsible for creating new entities, components, deletion ...
 */
class World
{
  public:
    World(ComponentsManager& components_manager, EntityManager& entity_manager)
        : componentsManager(components_manager)
        , entityManager(entity_manager)
    {
    }

    /**
     * @brief Create a new entity with these components
     * @tparam Components Component types
     * @param components Components to add to this entity
     * @return New entity id
     */
    template<typename... Components>
    Entity Spawn(Components... components);

    /**
     * @brief Destroy this entity
     * @param entity Entity to destroy
     */
    void Destroy(Entity entity);

    /**
     * @brief Add a new component to this entity
     * @tparam Component Component type
     * @param entity Entity to add to
     * @param component Component to add
     */
    template<typename Component>
    void AddComponent(Entity entity, Component component);

    /**
     * Remove a component of this type from entity
     * @tparam Component Component type to remove
     * @param entity Entity id
     */
    template<typename Component>
    void RemoveComponent(Entity entity);

    /**
     *
     * @param parent
     * @param child
     */
    void AddChild(Entity parent, Entity child);

    /**
     * @brief Flush changes to the world state
     * @return A checking whether any changes were made by the last system
     */
    bool Flush();

  private:
    class Command
    {
      public:
        virtual ~Command() = default;

        virtual void Execute() = 0;
    };

    template<typename T>
    class AddCommand;

    template<typename T>
    class RemoveCommand;

    void ExecuteDestroy(Entity entity) const;

    ComponentsManager& componentsManager;
    EntityManager& entityManager;
    std::vector<std::unique_ptr<Command>> toAdd;
    std::vector<std::unique_ptr<Command>> toRemove;
    std::vector<Entity> toDestroy;
};

template<typename T>
class World::AddCommand final : public World::Command
{
  public:
    AddCommand(const Entity ent, T comp, World& wld)
        : entity(ent)
        , component(comp)
        , world(wld) {};

    void Execute() override;

  private:
    Entity entity;
    T component;
    World& world;
};

template<typename T>
class World::RemoveCommand final : public World::Command
{
  public:
    RemoveCommand(const Entity ent, World& wld)
        : entity(ent)
        , world(wld) {};

    void Execute() override;

  private:
    Entity entity;
    World& world;
};

template<typename... Components>
inline Entity
World::Spawn(Components... components)
{
    auto entity = entityManager.AllocateEntity();
    ((void)AddComponent(entity, std::forward<Components>(components)), ...);

    // lastly check for core components, add if not already there

    constexpr bool hasPosition = HasTypeT<Position, Components...>();
    constexpr bool hasRotation = HasTypeT<Rotation, Components...>();
    constexpr bool hasScale = HasTypeT<Scale, Components...>();

    if (!hasPosition) {
        AddComponent(entity, Position{});
    }
    if (!hasRotation) {
        AddComponent(entity, Rotation{});
    }
    if (!hasScale) {
        AddComponent(entity, Scale{});
    }

    return entity;
}

template<typename Component>
inline void
World::AddComponent(Entity entity, Component component)
{
    toAdd.push_back(std::make_unique<AddCommand<Component>>(
        entity, std::forward<Component>(component), *this));
}

template<typename Component>
inline void
World::RemoveComponent(Entity entity)
{
    toRemove.push_back(
        std::make_unique<RemoveCommand<Component>>(entity, *this));
}

template<typename T>
inline void
World::AddCommand<T>::Execute()
{
    world.componentsManager.AddComponent(entity, component);
}

template<typename T>
inline void
World::RemoveCommand<T>::Execute()
{
    world.componentsManager.RemoveComponent<T>(entity);
}
}
