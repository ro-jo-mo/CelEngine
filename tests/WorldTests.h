#pragma once

#include "TestHelpers.h"

#include "core/App.h"

#include <gtest/gtest.h>

using namespace Cel;

inline void
SpawnEntitiesWithoutDefaults(Resource<World>& world)
{
    world->Spawn(GlobalTransform{ glm::mat4(1.0) });

    world->Spawn(Position{ glm::mat4(1.0) });

    world->Spawn(Rotation{ glm::mat4(1.0) });

    world->Spawn(Scale{ glm::mat4(1.0) });
}

inline void
EnsureEntitiesHaveDefaults(
    Query<With<GlobalTransform, Position, Rotation, Scale>>& query)
{
    size_t total = 0;
    for ( auto x : query) {
        total++;
    }

    ASSERT_EQ(total, 4)
        << "Not all entities had required default components inserted";
}

inline void
SpawnEntitiesWithKnownValues(Resource<World>& world)
{
    world->Spawn(KnownValues::rotation, KnownValues::testStruct);
}

inline void
EnsureEntitiesHaveKnownValues(
    Query<With<Rotation, KnownValues::TestStruct>>& query)
{
    for (auto [rot, t] : query) {
        ASSERT_EQ(rot.rotation, KnownValues::rotation.rotation)
            << "Rotation has changed after insertion";
        ASSERT_EQ(t.name, KnownValues::testStruct.name)
            << "TestStruct has changed after insertion";
        ASSERT_EQ(t.list, KnownValues::testStruct.list)
            << "TestStruct has changed after insertion";

        break;
    }
}

// Test AddComponent
inline Entity gAddRemoveEntity = 0;

inline void
SpawnEntityForAddRemove(Resource<World>& world)
{
    gAddRemoveEntity = world->Spawn().Get();
}

inline void
AddHealthComponent(Resource<World>& world)
{
    world->AddComponent(gAddRemoveEntity, Health{ 42 });
}

inline void
VerifyHealthAdded(Query<With<Health>>& query)
{
    ASSERT_TRUE(query.Has(gAddRemoveEntity))
        << "Health component should be present after AddComponent";
    auto [h] = query.Get(gAddRemoveEntity);
    ASSERT_EQ(h.value, 42);
}

// Test RemoveComponent
inline void
RemoveHealthComponent(Resource<World>& world)
{
    world->RemoveComponent<Health>(gAddRemoveEntity);
}

inline void
VerifyHealthRemoved(Query<With<Health>>& query)
{
    ASSERT_FALSE(query.Has(gAddRemoveEntity))
        << "Health component should be absent after RemoveComponent";
}

inline Entity gDestroyedEntity = 0;

inline void
SpawnAndDestroyEntity(Resource<World>& world)
{
    gDestroyedEntity = world->Spawn(Health{ 42 }).Get();
    world->Destroy(gDestroyedEntity);
}

inline void
VerifyEntityDestroyed(Query<With<Health>>& query)
{
    ASSERT_FALSE(query.Has(gDestroyedEntity))
        << "Destroyed entity should not appear in query";
}

// Test AddChild
inline Entity gParentEntity = 0;
inline Entity gChildEntity = 0;

inline void
SpawnParentAndChild(Resource<World>& world)
{
    gParentEntity = world->Spawn().Get();
    gChildEntity = world->Spawn().Get();
    world->AddChild(gParentEntity, gChildEntity);
}

inline void
VerifyChildAdded(Query<With<Children>>& parentQuery,
                 Query<With<Parent>>& childQuery)
{
    ASSERT_TRUE(parentQuery.Has(gParentEntity))
        << "Parent should have Children component";
    ASSERT_TRUE(childQuery.Has(gChildEntity))
        << "Child should have Parent component";

    auto [children] = parentQuery.Get(gParentEntity);
    ASSERT_TRUE(children.children.contains(gChildEntity))
        << "Parent's Children should contain child entity";

    auto [parent] = childQuery.Get(gChildEntity);
    ASSERT_EQ(parent.parent, gParentEntity);
}

// Test RemoveChild
inline void
RemoveChildFromParent(Resource<World>& world)
{
    world->RemoveChild(gParentEntity, gChildEntity);
}

inline void
VerifyChildRemoved(Query<With<Children>>& parentQuery,
                   Query<With<Parent>>& childQuery)
{
    ASSERT_FALSE(parentQuery.Has(gParentEntity));

    ASSERT_FALSE(childQuery.Has(gChildEntity))
        << "Child should no longer have a Parent component";
}

// Test ChildBuilder
inline Entity gBuilderParent = 0;
inline Entity gBuilderChild = 0;

inline void
SpawnWithChildrenBuilder(Resource<World>& world)
{
    gBuilderParent = world->Spawn()
                         .WithChildren([&](ChildBuilder builder) {
                             gBuilderChild = builder.Spawn(Health{ 42 }).Get();
                         })
                         .Get();
}

inline void
VerifyBuilderHierarchy(Query<With<Children>>& parentQuery,
                       Query<With<Parent>>& childQuery,
                       Query<With<Health>>& healthQuery)
{
    ASSERT_TRUE(parentQuery.Has(gBuilderParent))
        << "Builder parent should have Children component";
    auto [children] = parentQuery.Get(gBuilderParent);
    ASSERT_TRUE(children.children.contains(gBuilderChild));

    ASSERT_TRUE(childQuery.Has(gBuilderChild));
    auto [parent] = childQuery.Get(gBuilderChild);
    ASSERT_EQ(parent.parent, gBuilderParent);

    ASSERT_TRUE(healthQuery.Has(gBuilderChild));
    auto [h] = healthQuery.Get(gBuilderChild);
    ASSERT_EQ(h.value, 42);
}
