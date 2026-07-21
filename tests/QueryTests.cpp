#include "QueryTests.h"

#include "core/CorePlugin.h"

namespace {

TEST(QueryTest, WithFilterMatchesCorrectEntities)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnMixedEntities>>()
        .add_plugin<
            SingleSystemPlugin<Cel::TearDown::Middle, CountHealthEntities>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(QueryTest, WithoutFilterExcludesTaggedEntities)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<
            SingleSystemPlugin<Startup::Start, SpawnEnabledAndDisabled>>()
        .add_plugin<
            SingleSystemPlugin<Cel::TearDown::Middle, CountEnabledHealth>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(QueryTest, HasReturnsTrueForMatchingEntity)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnForHasCheck>>()
        .add_plugin<SingleSystemPlugin<Cel::TearDown::Middle, CheckHas>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(QueryTest, GetReturnsCorrectComponentValues)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnForGet>>()
        .add_plugin<SingleSystemPlugin<Cel::TearDown::Middle, CheckGet>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(QueryTest, EntityTypeInWithList)
{

    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnForEntityQuery>>()
        .add_plugin<
            SingleSystemPlugin<Cel::TearDown::Middle, CheckEntityInQuery>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(QueryTest, MultiComponentIntersection)
{

    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnForIntersection>>()
        .add_plugin<
            SingleSystemPlugin<Cel::TearDown::Middle, CountIntersection>>();
    ecs.start<Startup>().end<Cel::TearDown>();

}

TEST(QueryTest, IteratorAllowsComponentMutation)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnForMutation>>()
        .add_plugin<SingleSystemPlugin<Startup::PostStart, MutateHealth>>()
        .add_plugin<SingleSystemPlugin<Cel::TearDown::Middle, VerifyMutation>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

}
