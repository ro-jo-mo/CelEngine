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

TEST(AddRemoveComponentsTest, AddComponent)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<
            SingleSystemPlugin<Startup::First, SpawnEntityForAddRemove>>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, AddHealthComponent>>()
        .AddPlugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyHealthAdded>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(AddRemoveComponentsTest, RemoveComponent)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<
            SingleSystemPlugin<Startup::First, SpawnEntityForAddRemove>>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, AddHealthComponent>>()
        .AddPlugin<
            SingleSystemPlugin<Startup::PostStart, RemoveHealthComponent>>()
        .AddPlugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyHealthRemoved>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(WorldTest, DestroyEntity)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnAndDestroyEntity>>()
        .AddPlugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyEntityDestroyed>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(HierarchyTest, AddChild)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnParentAndChild>>()
        .AddPlugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyChildAdded>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(HierarchyTest, RemoveChild)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnParentAndChild>>()
        .AddPlugin<
            SingleSystemPlugin<Startup::PostStart, RemoveChildFromParent>>()
        .AddPlugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyChildRemoved>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(HierarchyTest, WithChildrenBuilder)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<
            SingleSystemPlugin<Startup::Start, SpawnWithChildrenBuilder>>()
        .AddPlugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      VerifyBuilderHierarchy>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

}
