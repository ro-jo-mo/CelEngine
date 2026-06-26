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
    for (const auto& x : query) {
        total++;
    }

    ASSERT_EQ(total, 4)
        << "Not all entities had required default components inserted";
}

;

inline void
SpawnEntitiesWithKnownValues(Resource<World>& world)
{
    world->Spawn(KnownValues::rotation, KnownValues::testStruct);
}

inline void
EnsureEntitiesHaveKnownValues(
    Query<With<Rotation, KnownValues::TestStruct>>& query)
{
    for (const auto& [rot, t] : query) {
        ASSERT_EQ(rot.rotation, KnownValues::rotation.rotation)
            << "Rotation has changed after insertion";
        ASSERT_EQ(t.name, KnownValues::testStruct.name)
            << "TestStruct has changed after insertion";
        ASSERT_EQ(t.list, KnownValues::testStruct.list)
            << "TestStruct has changed after insertion";

        break;
    }
}