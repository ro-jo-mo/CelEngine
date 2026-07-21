#include "HierarchyTests.h"

#include "core/CorePlugin.h"

namespace {

TEST(HierarchyPropagationTest, ChildInheritsParentTransform)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start,
                                      SpawnParentChildForPropagation>>()
        .add_plugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      VerifyChildInheritsParentTransform>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(HierarchyPropagationTest, GrandchildAccumulatesTransform)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnDeepHierarchy>>()
        .add_plugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      VerifyGrandchildTransform>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(HierarchyPropagationTest, ScalePropagatesDownHierarchy)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnScaledHierarchy>>()
        .add_plugin<
            SingleSystemPlugin<Cel::TearDown::Middle, VerifyChildWorldScale>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

TEST(HierarchyPropagationTest, StandaloneEntityGlobalTransformMatchesLocal)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<SingleSystemPlugin<Startup::Start, SpawnStandaloneEntity>>()
        .add_plugin<SingleSystemPlugin<Cel::TearDown::Middle,
                                      VerifyStandaloneGlobalTransform>>();
    ecs.start<Startup>().end<Cel::TearDown>();
}

}
