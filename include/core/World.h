#pragma once

#include "core/Transform.h"
#include "ecs/ComponentsManager.h"
#include "ecs/EntityManager.h"
#include "ecs/Helpers.h"
#include "ecs/Types.h"
#include <vector>

namespace Cel {
class EntityBuilder;
/**
 * @brief Manager of the game world state
 * Responsible for creating new entities, components, deletion ...
 */
class World
{
  public:
    World(ComponentsManager& componentsManager, EntityManager& entityManager)
        : componentsManager(componentsManager)
        , entityManager(entityManager)
    {
    }

    /**
     * @brief Create a new entity with these components
     * @tparam Components Component types
     * @param components Components to add to this entity
     * @return New entity id
     */
    template<typename... Components>
    EntityBuilder spawn(Components... components);

    /**
     * @brief Destroy this entity
     * @param entity Entity to destroy
     */
    void destroy(Entity entity);

    /**
     * @brief Add a new component to this entity
     * @tparam Component Component type
     * @param entity Entity to add to
     * @param component Component to add
     */
    template<typename Component>
    void add_component(Entity entity, Component component);

    /**
     * Remove a component of this type from entity
     * @tparam Component Component type to remove
     * @param entity Entity id
     */
    template<typename Component>
    void remove_component(Entity entity);

    /**
     * @brief Add "parent" as a parent entity to "child", and "child" as a child
     * object to "parent"
     * @param parent parent entity id
     * @param child child entity id
     */
    void add_child(Entity parent, Entity child);

    /**
     * @brief Remove child's parent component, and remove child from parent's
     * children list
     * @param parent parent entity id
     * @param child child entity id
     */
    void remove_child(Entity parent, Entity child);

    /**
     * @brief Flush changes to the world state
     * @return A checking whether any changes were made by the last system
     */
    bool flush();

  private:
    class Command
    {
      public:
        virtual ~Command() = default;

        virtual void execute() = 0;
    };

    template<typename T>
    class AddCommand;

    template<typename T>
    class RemoveCommand;

    class AddChildCommand
    {
      public:
        AddChildCommand(const Entity parent, const Entity child, World& world)
            : parent(parent)
            , child(child)
            , world(world) {};
        void execute() const;

      private:
        Entity parent;
        Entity child;
        std::reference_wrapper<World> world;
    };

    class RemoveChildCommand
    {
      public:
        RemoveChildCommand(const Entity parent,
                           const Entity child,
                           World& world)
            : parent(parent)
            , child(child)
            , world(world) {};

        void execute() const;

      private:
        Entity parent;
        Entity child;
        std::reference_wrapper<World> world;
    };

    void execute_destroy(Entity entity) const;

    ComponentsManager& componentsManager;
    EntityManager& entityManager;
    std::vector<std::unique_ptr<Command>> toAdd;
    std::vector<std::unique_ptr<Command>> toRemove;
    std::vector<AddChildCommand> toAddChild{};
    std::vector<RemoveChildCommand> toRemoveChild;
    std::vector<Entity> toDestroy;
};

class EntityBuilder
{
  public:
    EntityBuilder(Entity entity, World& world)
        : entity(entity)
        , world(world)
    {
    }

    [[nodiscard]] Entity get() const;
    EntityBuilder with_children(auto func);

  private:
    Entity entity;
    World& world;
};

class ChildBuilder
{
  public:
    ChildBuilder(const Entity parent, World& world)
        : parent(parent)
        , world(world)
    {
    }

    [[nodiscard]] Entity get() const;

    template<typename... Components>
    EntityBuilder spawn(Components... components);

  private:
    Entity parent;
    World& world;
};

template<typename T>
class World::AddCommand final : public Command
{
  public:
    AddCommand(const Entity ent, T comp, World& wld)
        : entity(ent)
        , component(comp)
        , world(wld) {};

    void execute() override;

  private:
    Entity entity;
    T component;
    World& world;
};

template<typename T>
class World::RemoveCommand final : public Command
{
  public:
    RemoveCommand(const Entity ent, World& wld)
        : entity(ent)
        , world(wld) {};

    void execute() override;

  private:
    Entity entity;
    World& world;
};

template<typename... Components>
inline EntityBuilder
World::spawn(Components... components)
{
    auto entity = entityManager.allocate_entity();
    ((void)add_component(entity, std::forward<Components>(components)), ...);

    // lastly check for core components, add if not already there
    constexpr bool hasPosition = has_type_t<Position, Components...>();
    constexpr bool hasRotation = has_type_t<Rotation, Components...>();
    constexpr bool hasScale = has_type_t<Scale, Components...>();
    constexpr bool hasGlobalTransform =
        has_type_t<GlobalTransform, Components...>();

    if (!hasPosition) {
        add_component(entity, Position{});
    }
    if (!hasRotation) {
        add_component(entity, Rotation{});
    }
    if (!hasScale) {
        add_component(entity, Scale{});
    }
    if (!hasGlobalTransform) {
        add_component(entity, GlobalTransform{});
    }

    return { entity, *this };
}

template<typename Component>
inline void
World::add_component(Entity entity, Component component)
{
    toAdd.push_back(std::make_unique<AddCommand<Component>>(
        entity, std::forward<Component>(component), *this));
}

template<typename Component>
inline void
World::remove_component(Entity entity)
{
    toRemove.push_back(
        std::make_unique<RemoveCommand<Component>>(entity, *this));
}

template<typename T>
inline void
World::AddCommand<T>::execute()
{
    world.componentsManager.add_component(entity, component);
}

template<typename T>
inline void
World::RemoveCommand<T>::execute()
{
    world.componentsManager.remove_component<T>(entity);
}

EntityBuilder
EntityBuilder::with_children(auto func)
{
    func(ChildBuilder(entity, world));

    return EntityBuilder{ entity, world };
}

template<typename... Components>
EntityBuilder
ChildBuilder::spawn(Components... components)
{
    auto child = world.spawn(components...).get();
    world.add_child(parent, child);

    return EntityBuilder(child, world);
}
}
