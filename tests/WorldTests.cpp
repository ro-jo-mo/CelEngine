#include "WorldTests.h"

#include "core/CorePlugin.h"

namespace {

TEST(SpawnTest, NoComponents)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<
            SingleSystemPlugin<Startup::Start, SpawnEntitiesWithoutDefaults>>()
        .AddPlugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      EnsureEntitiesHaveDefaults>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(SpawnTest, ComponentValues)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<
            SingleSystemPlugin<Startup::Start, SpawnEntitiesWithKnownValues>>()
        .AddPlugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      EnsureEntitiesHaveKnownValues>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(AddRemoveComponentsTest, AddComponent) {}

TEST(AddRemoveComponentsTest, RemoveComponent) {}

TEST(HierarchyTest, AddChild) {}

}
