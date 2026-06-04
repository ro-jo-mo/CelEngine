#include "core/World.h"

using namespace Cel;

void
World::Destroy(const Entity entity)
{
    toDestroy.push_back(entity);
}

void
World::AddChild(const Entity parent, const Entity child)
{
    toAddChild.emplace_back(parent, child, *this);
}

void
World::RemoveChild(const Entity parent, const Entity child)
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
    if (componentsManager.HasComponent<Children>(entity)) {
        // If so, delete their children
        const auto [children] =
            componentsManager.GetComponent<Children>(entity);

        for (auto& child : children) {
            RecurseThroughChildren(child, toRemove, componentsManager);
        }
    }
}

bool
World::Flush()
{
    // order
    // add
    // remove
    // destroy
    for (const auto& cmd : toAdd) {
        cmd->Execute();
    }
    for (const auto& cmd : toAddChild) {
        cmd.Execute();
    }
    for (const auto& cmd : toRemove) {
        cmd->Execute();
    }
    for (const auto& cmd : toRemoveChild) {
        cmd.Execute();
    }

    // Destroy all child entities of destroyed entity
    std::unordered_set<Entity> toDestroySet;
    for (const auto& entity : toDestroy) {
        RecurseThroughChildren(entity, toDestroySet, componentsManager);
    }
    for (const auto& entity : toDestroySet) {
        ExecuteDestroy(entity);
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
World::ExecuteDestroy(const Entity entity) const
{
    entityManager.DestroyEntity(entity);
    componentsManager.DestroyEntity(entity);
}
void
World::AddChildCommand::Execute() const
{
    // If parent has no children, add component
    if (!world.componentsManager.HasComponent<Children>(parent)) {
        world.componentsManager.AddComponent(parent, Children{});
    }

    if (world.componentsManager.HasComponent<Parent>(child)) {
        throw std::runtime_error(
            "Child entity already has a parent! Unparent the object first if "
            "you want to change its parent");
    }

    world.componentsManager.AddComponent(child, Parent{ parent });
    auto [children] = world.componentsManager.GetComponent<Children>(parent);
    children.insert(child);
}
void
World::RemoveChildCommand::Execute() const
{
    world.componentsManager.RemoveComponent<Parent>(child);
    auto [children] = world.componentsManager.GetComponent<Children>(parent);
    children.erase(child);
    if (children.empty()) {
        world.componentsManager.RemoveComponent<Children>(parent);
    }
}
Entity
EntityBuilder::Get() const
{
    return entity;
}
