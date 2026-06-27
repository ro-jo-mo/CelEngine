#include "ResourceManagerTests.h"

#include "core/CorePlugin.h"

namespace {

TEST(ResourceManagerTest, InsertAndGetIntResource)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<ResourcePlugin<IntResource, 42>>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, ReadIntResource>>();

    ecs.Start<Startup>();
}

TEST(ResourceManagerTest, ResourceMutationPersists)
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<ResourcePlugin<IntResource, 42>>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, MutateIntResource>>()
        .AddPlugin<
            SingleSystemPlugin<Startup::PostStart, VerifyMutatedIntResource>>();
    ecs.Start<Startup>();
}

}
