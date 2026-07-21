#include "ResourceManagerTests.h"

#include "core/CorePlugin.h"

namespace {

TEST(ResourceManagerTest, InsertAndGetIntResource)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<ResourcePlugin<IntResource, 42>>()
        .add_plugin<SingleSystemPlugin<Startup::Start, ReadIntResource>>();

    ecs.start<Startup>();
}

TEST(ResourceManagerTest, ResourceMutationPersists)
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<ResourcePlugin<IntResource, 42>>()
        .add_plugin<SingleSystemPlugin<Startup::Start, MutateIntResource>>()
        .add_plugin<
            SingleSystemPlugin<Startup::PostStart, VerifyMutatedIntResource>>();
    ecs.start<Startup>();
}

}
