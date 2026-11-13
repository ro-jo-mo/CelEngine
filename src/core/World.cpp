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

        throw_error("Attempted to add the same component twice to the same "
                    "entity in a single world flush!");

        // In retrospect this will probably be confusing, so I'll just error
        // archetypeManager.get_component_info(cmd.component)
        //    .destructor(cmd.data, 1);
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

        // If the components exists we can simply add the child
        archetypeManager.get_component_for_entity<Children>(cmd.parent)
            ->children.emplace(cmd.child);
    } else {
        // If the entity lacks the component we need to check if we're adding
        // multiple children this flush
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
                arena.allocate(sizeof(Children), alignof(Children)).get();

            Children children{ .children = { cmd.child } };

            archetypeManager.get_component_info(component_id<Children>())
                .move(&children, ptr);

            info.datas[component_id<Children>()] = ptr;
            info.signature.set(component_id<Children>(), true);
        }
    }

    auto& originalSignature = archetypeManager.get_entity_signature(cmd.parent);

    if (originalSignature.test(component_id<Parent>())) {
        throw_error(
            "Child entity already has a parent! Unparent the object first if "
            "you want to change its parent");
    }

    auto& info =
        updateInfo
            .emplace(cmd.parent, UpdateInfo{ .signature = originalSignature })
            .first->second;

    if (info.signature.test(component_id<Parent>())) {
        throw_error("Attempted to add a parent to the same entity twice in the "
                    "same world flush!");
    } else {

        const auto ptr = arena.allocate(sizeof(Parent), alignof(Parent)).get();

        Parent parent{ .parent = cmd.parent };

        archetypeManager.get_component_info(component_id<Parent>())
            .move(&parent, ptr);

        info.datas[component_id<Parent>()] = ptr;
        info.signature.set(component_id<Parent>(), true);
    }
}

void
World::execute_remove_child(const RemoveChild& cmd,
                            std::unordered_map<Entity, UpdateInfo>& updateInfo)
{
    // Remove child component only if there are no children left
    // Otherwise just erase child from list
    {
        const auto& originalSignature =
            archetypeManager.get_entity_signature(cmd.parent);

        if (!originalSignature.test(component_id<Children>())) {
            throw_error(
                "Attempted to remove parent from entity with no parent");
        }

        auto& children =
            *static_cast<Children*>(archetypeManager.get_component_for_entity(
                cmd.parent, component_id<Children>()));

        if (!children.children.contains(cmd.child)) {
            throw_error("Attempted to remove a child entity  from an unrelated "
                        "parent!");
        }

        children.children.erase(cmd.child);

        if (children.children.empty()) {
            auto& info =
                updateInfo
                    .emplace(cmd.parent,
                             UpdateInfo{ .signature = originalSignature })
                    .first->second;

            info.signature.set(component_id<Children>(), false);
        }
    }

    {
        const auto& originalSignature =
            archetypeManager.get_entity_signature(cmd.child);

        if (!originalSignature.test(component_id<Parent>())) {
            throw_error(
                "Attempted to remove parent from entity with no parent");
        }

        auto& info = updateInfo
                         .emplace(cmd.child,
                                  UpdateInfo{ .signature = originalSignature })
                         .first->second;

        info.signature.set(component_id<Parent>(), false);
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
