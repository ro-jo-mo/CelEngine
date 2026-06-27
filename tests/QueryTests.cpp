#include "QueryTests.h"

#include "core/CorePlugin.h"

namespace {

TEST(QueryTest, WithFilterMatchesCorrectEntities)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnMixedEntities>>()
        .AddPlugin<
            SingleSystemPlugin<Cel::TearDown::Middle, CountHealthEntities>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(QueryTest, WithoutFilterExcludesTaggedEntities)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<
            SingleSystemPlugin<Startup::Start, SpawnEnabledAndDisabled>>()
        .AddPlugin<
            SingleSystemPlugin<Cel::TearDown::Middle, CountEnabledHealth>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(QueryTest, HasReturnsTrueForMatchingEntity)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnForHasCheck>>()
        .AddPlugin<SingleSystemPlugin<Cel::TearDown::Middle, CheckHas>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(QueryTest, GetReturnsCorrectComponentValues)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnForGet>>()
        .AddPlugin<SingleSystemPlugin<Cel::TearDown::Middle, CheckGet>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(QueryTest, EntityTypeInWithList)
{

    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnForEntityQuery>>()
        .AddPlugin<
            SingleSystemPlugin<Cel::TearDown::Middle, CheckEntityInQuery>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(QueryTest, MultiComponentIntersection)
{

    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnForIntersection>>()
        .AddPlugin<
            SingleSystemPlugin<Cel::TearDown::Middle, CountIntersection>>();
    ecs.Start<Startup>().End<Cel::TearDown>();

}

TEST(QueryTest, IteratorAllowsComponentMutation)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnForMutation>>()
        .AddPlugin<SingleSystemPlugin<Startup::PostStart, MutateHealth>>()
        .AddPlugin<SingleSystemPlugin<Cel::TearDown::Middle, VerifyMutation>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

}
