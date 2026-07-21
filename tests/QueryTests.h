#pragma once

#include "TestHelpers.h"

#include "core/App.h"

#include <gtest/gtest.h>

using namespace Cel;

// ========================================

// Ensure basic filtering works
inline void
SpawnMixedEntities(Resource<World>& world)
{
    world->spawn(Health{ 10 });
    world->spawn(Health{ 20 });
    world->spawn(Velocity{ 1.f, 0.f, 0.f });
    world->spawn(Health{ 30 }, Velocity{});
}

inline void
CountHealthEntities(Query<With<Health>>& query)
{
    size_t count = 0;
    for (const auto& [h] : query) {
        count++;
    }

    ASSERT_EQ(count, 3);
}

// ========================================

// Ensure exclude filtering works
inline void
SpawnEnabledAndDisabled(Resource<World>& world)
{
    world->spawn(Health{ 1 });
    world->spawn(Health{ 2 });
    world->spawn(Health{ 3 }, Disabled{});
}

inline void
CountEnabledHealth(Query<With<Health>, Without<Disabled>>& query)
{
    size_t count = 0;
    for (const auto& [h] : query) {
        count++;
    }

    ASSERT_EQ(2, count);
}

// ========================================
inline Entity gHasEntity = 0;
inline Entity gHasNoEntity = 0;
// Ensure "Has" works correctly
inline void
SpawnForHasCheck(Resource<World>& world)
{
    gHasEntity = world->spawn(Health{}).get();
    gHasNoEntity = world->spawn(Velocity{}).get();
}

inline void
CheckHas(Query<With<Health>>& query)
{
    ASSERT_TRUE(query.has(gHasEntity))
        << "Entity with Health should be found by Has()";
    ASSERT_FALSE(query.has(gHasNoEntity))
        << "Entity without Health should not be found by Has()";
}

// ========================================
inline Entity gGetEntity = 0;
// Ensure "Get" works
inline void
SpawnForGet(Resource<World>& world)
{
    gGetEntity = world->spawn(Health{ 77 }, Velocity{ 1.f, 2.f, 3.f }).get();
}

inline void
CheckGet(Query<With<Health, Velocity>>& query)
{
    ASSERT_TRUE(query.has(gGetEntity));
    auto [h, v] = query.get(gGetEntity);
    ASSERT_EQ(h.value, 77);
    ASSERT_FLOAT_EQ(v.x, 1.f);
    ASSERT_FLOAT_EQ(v.y, 2.f);
    ASSERT_FLOAT_EQ(v.z, 3.f);
}

// ========================================
inline Entity gEntityInQuery = 0;
// Ensure entity is found during query iter
inline void
SpawnForEntityQuery(Resource<World>& world)
{
    gEntityInQuery = world->spawn(Health{ 5 }).get();
}

inline void
CheckEntityInQuery(Query<With<Entity, Health>>& query)
{
    bool found = false;
    for (const auto& [e, h] : query) {
        if (e == gEntityInQuery) {
            found = true;
            ASSERT_EQ(h.value, 5);
        }
    }
    ASSERT_TRUE(found)
        << "Entity should have been yielded by Query<With<Entity, Health>>";
}

// ========================================

// Ensure retrieving multiple components works
inline void
SpawnForIntersection(Resource<World>& world)
{
    world->spawn(Health{});             // Health only
    world->spawn(Velocity{});           // Velocity only
    world->spawn(Health{}, Velocity{}); // both — only this matches
    world->spawn(Health{}, Velocity{}); // both — two matches total
}

inline void
CountIntersection(Query<With<Health, Velocity>>& query)
{
    size_t count = 0;
    for (const auto& [h, v] : query) {
        count++;
    }

    ASSERT_EQ(2, count);
}

// ========================================
inline Entity gMutateEntity = 0;
// Ensure mutated data is consistent between queries
inline void
SpawnForMutation(Resource<World>& world)
{
    gMutateEntity = world->spawn(Health{ 1 }).get();
}

inline void
MutateHealth(Query<With<Health>>& query)
{
    std::cout << "first" << std::endl;
    for (auto [h] : query) {
        std::cout << "here" << std::endl;
        h.value += 9;
    }
}

inline void
VerifyMutation(Query<With<Health>>& query)
{
    ASSERT_TRUE(query.has(gMutateEntity));
    auto [h] = query.get(gMutateEntity);

    ASSERT_EQ(h.value, 10);
}
