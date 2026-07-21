#pragma once

#include "TestHelpers.h"

#include "core/App.h"
#include "core/Transform.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>

#include <gtest/gtest.h>

using namespace Cel;

static constexpr float kEpsilon = 1e-4f;

inline bool
Vec3Near(const glm::vec3& a, const glm::vec3& b)
{
    return glm::all(glm::epsilonEqual(a, b, kEpsilon));
}

// Quaternions q and -q represent the same rotation
inline bool
QuatNear(const glm::quat& a, const glm::quat& b)
{
    return glm::all(glm::epsilonEqual(a, b, kEpsilon)) ||
           glm::all(glm::epsilonEqual(a, -b, kEpsilon));
}

// ========================================
inline Entity gRootEntity = 0;

// Ensure a root nodes global transform is simply its local transform
inline void
SpawnRootWithPosition(Resource<World>& world)
{
    gRootEntity = world
                      ->spawn(Position{ 1.f, 2.f, 3.f },
                              Rotation{ 0.f, glm::half_pi<float>(), 0.f },
                              Scale{ 2.f })
                      .with_children([&](ChildBuilder b) {
                          b.spawn(Position{ 0.f, 0.f, 0.f });
                      })
                      .get();
}

inline void
VerifyRootGlobalTransform(Query<With<GlobalTransform,
                                     const Position,
                                     const Rotation,
                                     const Scale,
                                     const Children>,
                                Without<Parent>>& rootQuery)
{
    ASSERT_TRUE(rootQuery.has(gRootEntity));
    auto [gt, pos, rot, scale, children] = rootQuery.get(gRootEntity);

    const glm::vec3 translation = gt.get_translation();
    ASSERT_TRUE(Vec3Near(translation, glm::vec3(1.f, 2.f, 3.f)))
        << "Root GlobalTransform translation should match local Position";

    const glm::quat worldRot = gt.get_rotation();
    const glm::quat expectedRot{ glm::vec3(0.f, glm::half_pi<float>(), 0.f) };
    ASSERT_TRUE(QuatNear(worldRot, expectedRot))
        << "Root GlobalTransform rotation should match local Rotation";

    const glm::vec3 worldScale = gt.get_scale();
    ASSERT_TRUE(Vec3Near(worldScale, glm::vec3(2.f, 2.f, 2.f)))
        << "Root GlobalTransform scale should match local Scale";
}

// ========================================
inline Entity gChildOfRoot = 0;

// Ensure the full transform (translation, rotation, scale) is propagated to a
// child entity
inline void
SpawnParentChildForPropagation(Resource<World>& world)
{
    gChildOfRoot = 0;
    world
        ->spawn(Position{ 10.f, 0.f, 0.f },
                Rotation{ 0.f, glm::half_pi<float>(), 0.f },
                Scale{ 2.f })
        .with_children([&](ChildBuilder b) {
            gChildOfRoot = b.spawn(Position{ 5.f, 0.f, 0.f },
                                   Rotation{ 0.f, glm::half_pi<float>(), 0.f })
                               .get();
        });
}

inline void
VerifyChildInheritsParentTransform(
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>>&
        childQuery)
{
    ASSERT_TRUE(childQuery.has(gChildOfRoot));

    auto [gt, pos, rot, scale] = childQuery.get(gChildOfRoot);

    // Parent: pos(10,0,0), rot(90°Y), scale(2). Child local: pos(5,0,0),
    // rot(90°Y). Parent 90°Y rotates child offset (5,0,0) → (0,0,-5), parent
    // scale 2 → (0,0,-10),
    // + parent translation (10,0,0) = (10,0,-10).
    const glm::vec3 worldPos = gt.get_translation();
    ASSERT_TRUE(Vec3Near(worldPos, glm::vec3(10.f, 0.f, -10.f)))
        << "Child world position should reflect parent rotation applied to "
           "local offset. Got: ("
        << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")";

    // world rot = parent_rot(90°Y) * child_local_rot(90°Y) = 180°Y
    const glm::quat worldRot = gt.get_rotation();
    const glm::quat expectedRot{ glm::vec3(0.f, glm::pi<float>(), 0.f) };
    ASSERT_TRUE(QuatNear(worldRot, expectedRot))
        << "Child world rotation should be parent_rot * child_local_rot";

    // world scale = parent_scale(2) * child_local_scale(1) = (2,2,2)
    const glm::vec3 worldScale = gt.get_scale();
    ASSERT_TRUE(Vec3Near(worldScale, glm::vec3(2.f, 2.f, 2.f)))
        << "Child world scale should be parent scale * child local scale";
}
// ========================================
inline Entity gGrandchild = 0;

// Ensure the full transform accumulates correctly through a 3-level hierarchy
inline void
SpawnDeepHierarchy(Resource<World>& world)
{
    gGrandchild = 0;
    world->spawn(Position{ 1.f, 0.f, 0.f }).with_children([&](ChildBuilder b) {
        b.spawn(Position{ 2.f, 0.f, 0.f },
                Rotation{ 0.f, glm::half_pi<float>(), 0.f },
                Scale{ 2.f })
            .with_children([&](ChildBuilder b2) {
                gGrandchild =
                    b2.spawn(Position{ 3.f, 0.f, 0.f },
                             Rotation{ 0.f, glm::half_pi<float>(), 0.f })
                        .get();
            });
    });
}

inline void
VerifyGrandchildTransform(
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>>&
        childQuery)
{
    ASSERT_TRUE(childQuery.has(gGrandchild));

    auto [gt, pos, rot, scale] = childQuery.get(gGrandchild);

    // Root: pos(1,0,0), identity rot, scale 1.
    // Middle: local pos(2,0,0), rot(90°Y), scale 2.
    //   Middle world pos = (2,0,0) + (1,0,0) = (3,0,0). World rot = 90°Y. World
    //   scale = 2.
    // Grandchild: local pos(3,0,0), rot(90°Y).
    //   Middle 90°Y rotates grandchild offset (3,0,0) → (0,0,-3), scaled by 2 →
    //   (0,0,-6),
    //   + middle world pos (3,0,0) = (3,0,-6).
    const glm::vec3 worldPos = gt.get_translation();
    ASSERT_TRUE(Vec3Near(worldPos, glm::vec3(3.f, 0.f, -6.f)))
        << "Grandchild world position should reflect accumulated rotations and "
           "scales. Got: ("
        << worldPos.x << ", " << worldPos.y << ", " << worldPos.z << ")";

    // world rot = middle_world_rot(90°Y) * grandchild_local_rot(90°Y) = 180°Y
    const glm::quat worldRot = gt.get_rotation();
    const glm::quat expectedRot{ glm::vec3(0.f, glm::pi<float>(), 0.f) };
    ASSERT_TRUE(QuatNear(worldRot, expectedRot))
        << "Grandchild world rotation should be accumulated from the hierarchy";

    // root scale 1, middle scale 2, grandchild scale 1 → world scale (2,2,2)
    const glm::vec3 worldScale = gt.get_scale();
    ASSERT_TRUE(Vec3Near(worldScale, glm::vec3(2.f, 2.f, 2.f)))
        << "Grandchild world scale should be accumulated from the hierarchy";
}

// ========================================
inline Entity gScaledChild = 0;

// Ensure scaling is propagated
inline void
SpawnScaledHierarchy(Resource<World>& world)
{
    gScaledChild = 0;
    world->spawn(Scale{ 2.f }).with_children([&](ChildBuilder b) {
        gScaledChild = b.spawn(Scale{ 3.f }).get();
    });
}

inline void
VerifyChildWorldScale(
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>>&
        childQuery)
{
    ASSERT_TRUE(childQuery.has(gScaledChild));

    auto [gt, pos, rot, scale] = childQuery.get(gScaledChild);
    const glm::vec3 worldScale = gt.get_scale();

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
    gStandaloneEntity = world
                            ->spawn(Position{ 7.f, 8.f, 9.f },
                                    Rotation{ 0.f, glm::half_pi<float>(), 0.f },
                                    Scale{ 3.f })
                            .get();
}

inline void
VerifyStandaloneGlobalTransform(
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>,
          Without<Parent, Children>>& rootQuery)
{
    ASSERT_TRUE(rootQuery.has(gStandaloneEntity));
    auto [gt, pos, rot, scale] = rootQuery.get(gStandaloneEntity);
    const glm::vec3 translation = gt.get_translation();
    ASSERT_TRUE(Vec3Near(translation, glm::vec3(7.f, 8.f, 9.f)))
        << "Standalone entity translation should match local Position";

    const glm::quat worldRot = gt.get_rotation();
    const glm::quat expectedRot{ glm::vec3(0.f, glm::half_pi<float>(), 0.f) };
    ASSERT_TRUE(QuatNear(worldRot, expectedRot))
        << "Standalone entity rotation should match local Rotation";

    const glm::vec3 worldScale = gt.get_scale();
    ASSERT_TRUE(Vec3Near(worldScale, glm::vec3(3.f, 3.f, 3.f)))
        << "Standalone entity scale should match local Scale";
}
