#pragma once

#include "Arena.h"
#include "core/Transform.h"
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
    World(ArchetypeManager& archetypeManager, EntityManager& entityManager)
        : archetypeManager(archetypeManager)
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
    struct AddComponent
    {
        Entity entity;
        ComponentId component;
        void* data;
    };

    struct RemoveComponent
    {
        Entity entity;
        ComponentId component;
    };

    struct AddChild
    {
        Entity parent;
        Entity child;
    };

    struct RemoveChild
    {
        Entity parent;
        Entity child;
    };

    void execute_destroy(Entity entity) const;

    void execute_add_component(
        const AddComponent& cmd,
        std::unordered_map<Entity, ArchetypeManager::UpdateInfo>& updateInfo)
        const;

    void execute_remove_component(
        const RemoveComponent& cmd,
        std::unordered_map<Entity, ArchetypeManager::UpdateInfo>& updateInfo)
        const;

    void execute_add_child(
        const AddChild& cmd,
        std::unordered_map<Entity, ArchetypeManager::UpdateInfo>& updateInfo);

    void execute_remove_child(
        const RemoveChild& cmd,
        std::unordered_map<Entity, ArchetypeManager::UpdateInfo>& updateInfo);

    ArchetypeManager& archetypeManager;
    EntityManager& entityManager;

    Arena arena{};
    
    std::vector<AddComponent> toAdd;
    std::vector<RemoveComponent> toRemove;
    std::vector<AddChild> toAddChild{};
    std::vector<RemoveChild> toRemoveChild;
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

template<typename... Components>
EntityBuilder
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
void
World::add_component(Entity entity, Component component)
{
    auto ptr = arena.allocate(sizeof(Component), alignof(Component));

    archetypeManager.get_component_info(component_id<Component>())
        .move(&component, ptr.get());

    toAdd.push_back({ .entity = entity,
                      .component = component_id<Component>(),
                      .data = ptr });
}

template<typename Component>
void
World::remove_component(const Entity entity)
{
    toRemove.push_back(
        { .entity = entity, .component = component_id<Component>() });
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
