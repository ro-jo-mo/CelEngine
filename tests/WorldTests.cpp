#include "WorldTests.h"

#include "core/CorePlugin.h"

namespace {

TEST(SpawnTest, NoComponents)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<
            SingleSystemPlugin<Startup::Start, SpawnEntitiesWithoutDefaults>>()
        .add_plugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      EnsureEntitiesHaveDefaults>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(SpawnTest, ComponentValues)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<
            SingleSystemPlugin<Startup::Start, SpawnEntitiesWithKnownValues>>()
        .add_plugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      EnsureEntitiesHaveKnownValues>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(AddRemoveComponentsTest, AddComponent)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<
            SingleSystemPlugin<Startup::First, SpawnEntityForAddRemove>>()
        .add_plugin<SingleSystemPlugin<Startup::Start, AddHealthComponent>>()
        .add_plugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyHealthAdded>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(AddRemoveComponentsTest, RemoveComponent)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<
            SingleSystemPlugin<Startup::First, SpawnEntityForAddRemove>>()
        .add_plugin<SingleSystemPlugin<Startup::Start, AddHealthComponent>>()
        .add_plugin<
            SingleSystemPlugin<Startup::PostStart, RemoveHealthComponent>>()
        .add_plugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyHealthRemoved>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(WorldTest, DestroyEntity)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnAndDestroyEntity>>()
        .add_plugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyEntityDestroyed>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(HierarchyTest, AddChild)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnParentAndChild>>()
        .add_plugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyChildAdded>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(HierarchyTest, RemoveChild)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnParentAndChild>>()
        .add_plugin<
            SingleSystemPlugin<Startup::PostStart, RemoveChildFromParent>>()
        .add_plugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyChildRemoved>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(HierarchyTest, WithChildrenBuilder)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<
            SingleSystemPlugin<Startup::Start, SpawnWithChildrenBuilder>>()
        .add_plugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      VerifyBuilderHierarchy>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

}
