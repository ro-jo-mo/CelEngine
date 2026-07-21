#include "core/World.h"

using namespace Cel;

void
World::destroy(const Entity entity)
{
    toDestroy.push_back(entity);
}

void
World::add_child(const Entity parent, const Entity child)
{
    toAddChild.emplace_back(parent, child, *this);
}

void
World::remove_child(const Entity parent, const Entity child)
{
    toRemoveChild.emplace_back(parent, child, *this);
}

void
RecurseThroughChildren(const Entity entity,
                       std::unordered_set<Entity>& toRemove,
                       ComponentsManager& componentsManager)
{
    toRemove.insert(entity);
    // Does this entity have children?
    if (componentsManager.has_component<Children>(entity)) {
        // If so, delete their children
        const auto [children] =
            componentsManager.get_component<Children>(entity);

        for (auto& child : children) {
            RecurseThroughChildren(child, toRemove, componentsManager);
        }
    }
}

bool
World::flush()
{
    // order
    // add
    // remove
    // destroy
    for (const auto& cmd : toAdd) {
        cmd->execute();
    }
    for (const auto& cmd : toAddChild) {
        cmd.execute();
    }
    for (const auto& cmd : toRemove) {
        cmd->execute();
    }
    for (const auto& cmd : toRemoveChild) {
        cmd.execute();
    }

    // Destroy all child entities of destroyed entity
    std::unordered_set<Entity> toDestroySet;
    for (const auto& entity : toDestroy) {
        RecurseThroughChildren(entity, toDestroySet, componentsManager);
    }
    for (const auto& entity : toDestroySet) {
        execute_destroy(entity);
    }

    const auto changesMade = toAdd.size() + toRemove.size() + toDestroy.size() +
                                 toAddChild.size() + toRemoveChild.size() >
                             0;
    toAdd.clear();
    toRemove.clear();
    toAddChild.clear();
    toRemoveChild.clear();
    toDestroy.clear();

    return changesMade;
}

void
World::execute_destroy(const Entity entity) const
{
    entityManager.destroy_entity(entity);
    componentsManager.destroy_entity(entity);
}
void
World::AddChildCommand::execute() const
{
    // If parent has no children, add component
    if (!world.get().componentsManager.has_component<Children>(parent)) {
        world.get().componentsManager.add_component(parent, Children{});
    }

    if (world.get().componentsManager.has_component<Parent>(child)) {
        throw std::runtime_error(
            "Child entity already has a parent! Unparent the object first if "
            "you want to change its parent");
    }

    world.get().componentsManager.add_component(child, Parent{ parent });
    auto& [children] =
        world.get().componentsManager.get_component<Children>(parent);
    children.insert(child);
}
void
World::RemoveChildCommand::execute() const
{
    world.get().componentsManager.remove_component<Parent>(child);
    auto& [children] =
        world.get().componentsManager.get_component<Children>(parent);
    children.erase(child);
    if (children.empty()) {
        world.get().componentsManager.remove_component<Children>(parent);
    }
}
Entity
EntityBuilder::get() const
{
    return entity;
}
Entity
ChildBuilder::get() const
{
    return parent;
}
