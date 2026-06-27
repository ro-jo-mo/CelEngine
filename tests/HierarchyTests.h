#pragma once

#include "TestHelpers.h"

#include "core/App.h"
#include "core/Transform.h"

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>

#include <gtest/gtest.h>

using namespace Cel;

static constexpr float kEpsilon = 1e-4f;

// Avoid floating point variations
// Roughly close to each other
inline bool
Vec3Near(const glm::vec3& a, const glm::vec3& b)
{
    return glm::all(glm::epsilonEqual(a, b, kEpsilon));
}

// ========================================
inline Entity gRootEntity = 0;

// Ensure a root nodes global transform is simply its local transform
inline void
SpawnRootWithPosition(Resource<World>& world)
{
    gRootEntity = world->Spawn(Position{ 1.f, 2.f, 3.f })
                      .WithChildren([&](ChildBuilder b) {
                          b.Spawn(Position{ 0.f, 0.f, 0.f });
                      })
                      .Get();
}

inline void
VerifyRootGlobalTransform(Query<With<GlobalTransform,
                                     const Position,
                                     const Rotation,
                                     const Scale,
                                     const Children>,
                                Without<Parent>>& rootQuery)
{
    ASSERT_TRUE(rootQuery.Has(gRootEntity));
    auto [gt, pos, rot, scale, children] = rootQuery.Get(gRootEntity);

    const glm::vec3 translation = gt.GetTranslation();
    ASSERT_TRUE(Vec3Near(translation, glm::vec3(1.f, 2.f, 3.f)))
        << "Root GlobalTransform translation should match local Position";
}

// ========================================
inline Entity gChildOfRoot = 0;

// Ensure translation is propagated to a child entity
inline void
SpawnParentChildForPropagation(Resource<World>& world)
{
    gChildOfRoot = 0;
    world->Spawn(Position{ 10.f, 0.f, 0.f }).WithChildren([&](ChildBuilder b) {
        gChildOfRoot = b.Spawn(Position{ 5.f, 0.f, 0.f }).Get();
    });
}

inline void
VerifyChildInheritsParentTranslation(
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>>&
        childQuery)
{
    ASSERT_TRUE(childQuery.Has(gChildOfRoot));

    auto [gt, pos, rot, scale] = childQuery.Get(gChildOfRoot);
    const glm::vec3 worldPos = gt.GetTranslation();

    // child local (5,0,0) + parent local (10,0,0) = (15,0,0) in world space
    ASSERT_TRUE(Vec3Near(worldPos, glm::vec3(15.f, 0.f, 0.f)))
        << "Child world position should be parent pos + child local pos. Got: ("
        << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")";
}
// ========================================
inline Entity gGrandchild = 0;

// Ensure hierarchy is propagated throughout a deeper hierarchy
inline void
SpawnDeepHierarchy(Resource<World>& world)
{
    gGrandchild = 0;
    world->Spawn(Position{ 1.f, 0.f, 0.f }).WithChildren([&](ChildBuilder b) {
        b.Spawn(Position{ 2.f, 0.f, 0.f }).WithChildren([&](ChildBuilder b2) {
            gGrandchild = b2.Spawn(Position{ 3.f, 0.f, 0.f }).Get();
        });
    });
}

inline void
VerifyGrandchildTranslation(
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>>&
        childQuery)
{
    ASSERT_TRUE(childQuery.Has(gGrandchild));

    auto [gt, pos, rot, scale] = childQuery.Get(gGrandchild);
    const glm::vec3 worldPos = gt.GetTranslation();

    // 1 + 2 + 3 = 6 along X
    ASSERT_TRUE(Vec3Near(worldPos, glm::vec3(6.f, 0.f, 0.f)))
        << "Grandchild world position should be 6 along X. Got: (" << worldPos.x
        << ", " << worldPos.y << ", " << worldPos.z << ")";
}

// ========================================
inline Entity gScaledChild = 0;

// Ensure scaling is propagated
inline void
SpawnScaledHierarchy(Resource<World>& world)
{
    gScaledChild = 0;
    world->Spawn(Scale{ 2.f }).WithChildren([&](ChildBuilder b) {
        gScaledChild = b.Spawn(Scale{ 3.f }).Get();
    });
}

inline void
VerifyChildWorldScale(
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>>&
        childQuery)
{
    ASSERT_TRUE(childQuery.Has(gScaledChild));

    auto [gt, pos, rot, scale] = childQuery.Get(gScaledChild);
    const glm::vec3 worldScale = gt.GetScale();

    // parent scale 2 * child local scale 3 = 6
    ASSERT_TRUE(Vec3Near(worldScale, glm::vec3(6.f, 6.f, 6.f)))
        << "Child world scale should be parent_scale * child_local_scale. Got: "
           "("
        << worldScale.x << ", " << worldScale.y << ", " << worldScale.z << ")";
}

// ========================================
inline Entity gStandaloneEntity = 0;
// Ensure in the case of a single entity, its global transform is still computed
inline void
SpawnStandaloneEntity(Resource<World>& world)
{
    // A standalone entity has no parent and no children — its GlobalTransform
    // should simply reflect its own local position.
    // We manually add Children so HierarchyPropagation visits it as a root.
    gStandaloneEntity = world->Spawn(Position{ 7.f, 8.f, 9.f }).Get();
}

inline void
VerifyStandaloneGlobalTransform(
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>,
          Without<Parent, Children>>& rootQuery)
{
    ASSERT_TRUE(rootQuery.Has(gStandaloneEntity));
    auto [gt, pos, rot, scale] = rootQuery.Get(gStandaloneEntity);
    const glm::vec3 translation = gt.GetTranslation();
    ASSERT_TRUE(Vec3Near(translation, glm::vec3(7.f, 8.f, 9.f)));
}
