#include "HierarchyTests.h"

#include "core/CorePlugin.h"

namespace {

TEST(HierarchyPropagationTest, ChildInheritsParentTransform)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start,
                                      SpawnParentChildForPropagation>>()
        .AddPlugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      VerifyChildInheritsParentTransform>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(HierarchyPropagationTest, GrandchildAccumulatesTransform)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnDeepHierarchy>>()
        .AddPlugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      VerifyGrandchildTransform>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(HierarchyPropagationTest, ScalePropagatesDownHierarchy)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnScaledHierarchy>>()
        .AddPlugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyChildWorldScale>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

TEST(HierarchyPropagationTest, StandaloneEntityGlobalTransformMatchesLocal)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SpawnStandaloneEntity>>()
        .AddPlugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      VerifyStandaloneGlobalTransform>>();
    ecs.Start<Startup>().End<Cel::TearDown>();
}

}
