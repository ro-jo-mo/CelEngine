#include "core/World.h"

using namespace Cel;

using UpdateInfo = ArchetypeManager::UpdateInfo;

void
World::destroy(const Entity entity)
{
    toDestroy.push_back(entity);
}

void
World::add_child(const Entity parent, const Entity child)
{
    toAddChild.emplace_back(parent, child);
}

void
World::remove_child(const Entity parent, const Entity child)
{
    toRemoveChild.emplace_back(parent, child);
}

static void
add_children_to_destroy(const Entity entity,
                        std::unordered_set<Entity>& toRemove,
                        ArchetypeManager& manager)
{
    toRemove.insert(entity);
    // Does this entity have children?
    if (manager.has_component<Children>(entity)) {

        // If so, delete their children
        const auto [children] = *static_cast<Children*>(
            manager.get_component_for_entity(entity, component_id<Children>()));

        for (auto& child : children) {
            add_children_to_destroy(child, toRemove, manager);
        }
    }
}

void
World::execute_destroy(const Entity entity) const
{
    entityManager.destroy_entity(entity);
    archetypeManager.destroy_entity(entity);
}

void
World::execute_add_component(
    const AddComponent& cmd,
    std::unordered_map<Entity, UpdateInfo>& updateInfo) const
{
    // if we edit this entity, add new sig to map. if already exists in map
    // update it

    auto& originalSignature = archetypeManager.get_entity_signature(cmd.entity);

    auto [iter, _] = updateInfo.emplace(
        cmd.entity, UpdateInfo{ .signature = originalSignature });

    auto& info = iter->second;

    if (originalSignature.test(cmd.component)) {
        throw_error("Error! Attempted to add a component to an entity that "
                    "already has it!");
    }

    info.signature.set(cmd.component, true);

    if (info.datas[cmd.component] != nullptr) {
        // Call destructor
        archetypeManager.get_component_info(cmd.component)
            .destructor(cmd.data, 1);
    } else {
        info.datas[cmd.component] = cmd.data;
    }
}

void
World::execute_remove_component(
    const RemoveComponent& cmd,
    std::unordered_map<Entity, UpdateInfo>& updateInfo) const
{

    auto& originalSignature = archetypeManager.get_entity_signature(cmd.entity);

    auto [iter, justAdded] = updateInfo.emplace(
        cmd.entity, UpdateInfo{ .signature = originalSignature });

    auto& info = iter->second;

    if (!originalSignature.test(cmd.component)) {
        throw_error("Error! Attempted to remove a component from an entity "
                    "that does not"
                    "already have it!");
    }

    info.signature.set(cmd.component, false);
}

void
World::execute_add_child(const AddChild& cmd,
                         std::unordered_map<Entity, UpdateInfo>& updateInfo)
{
    // If parent has no children, add component
    if (archetypeManager.has_component<Children>(cmd.parent)) {

        // The logic is tricky here. If the component does not exist, it goes to
        // updateInfo. Otherwise edit the data in the archetype

        archetypeManager.get_component_for_entity<Children>(cmd.parent)
            ->children.emplace(cmd.child);
    } else {

        auto& originalSignature =
            archetypeManager.get_entity_signature(cmd.parent);

        auto& info = updateInfo
                         .emplace(cmd.parent,
                                  UpdateInfo{ .signature = originalSignature })
                         .first->second;

        // If we've already added a child, append to vector
        if (info.signature.test(component_id<Children>())) {
            static_cast<Children*>(info.datas[component_id<Children>()])
                ->children.insert(cmd.child);
        } else {
            // Otherwise create new child component
            const auto ptr =
                &childStore.emplace_back(std::unordered_set<Entity>(cmd.child));
            info.datas[component_id<Children>()] = ptr;
        }

        info.signature.set(cmd.component, true);
    }

    if (archetypeManager.has_component<Parent>(cmd.child)) {
        throw std::runtime_error(
            "Child entity already has a parent! Unparent the object first if "
            "you want to change its parent");
    }

    // Check we don't try to parent the same entity twice in the same flush
    if ()

        world.get().componentsManager.add_component(child, Parent{ parent });
    auto& [children] =
        world.get().componentsManager.get_component<Children>(parent);
    children.insert(child);
}

void
World::execute_remove_child(const RemoveChild& cmd,
                            std::unordered_map<Entity, UpdateInfo>& updateInfo)
{
    world.get().componentsManager.remove_component<Parent>(child);
    auto& [children] =
        world.get().componentsManager.get_component<Children>(parent);
    children.erase(child);
    if (children.empty()) {
        world.get().componentsManager.remove_component<Children>(parent);
    }
}

bool
World::flush()
{
    // order
    // add
    // remove
    // destroy

    std::unordered_map<Entity, UpdateInfo> updateInfo;

    for (const auto& cmd : toAdd) {
        execute_add_component(cmd, updateInfo);
    }

    // At this stage we reuse the toAdd list to store child data
    for (const auto& cmd : toAddChild) {
        execute_add_child(cmd, TODO);
    }

    for (const auto& cmd : toRemove) {
        execute_remove_component(cmd, updateInfo);
    }
    for (const auto& cmd : toRemoveChild) {
        execute_remove_child(cmd, TODO);
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
