#include "SchedulingTests.h"

#include "core/CorePlugin.h"

namespace {

TEST(SchedulingTest, AddSystemRunsOnSchedule)
{
    gExecutionLog.clear();

    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SingleSystem>>();
    ecs.Start<Startup>();

    ASSERT_EQ(gExecutionLog.size(), 1);
    ASSERT_EQ(gExecutionLog[0], 1);
}

TEST(SchedulingTest, AddChainEnforcesOrder)
{
    gExecutionLog.clear();

    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<ChainPlugin<Startup::Start, ChainFirst, ChainSecond>>();
    ecs.Start<Startup>();

    ASSERT_EQ(gExecutionLog.size(), 2);
    ASSERT_EQ(gExecutionLog[0], 10)
        << "ChainFirst should run before ChainSecond";
    ASSERT_EQ(gExecutionLog[1], 20)
        << "ChainSecond should run after ChainFirst";
}

TEST(SchedulingTest, AddGroupRunsAllSystems)
{
    gExecutionLog.clear();

    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<GroupPlugin<Startup::Start, GroupA, GroupB>>();
    ecs.Start<Startup>();

    ASSERT_EQ(gExecutionLog.size(), 2);

    const bool has100 = (gExecutionLog[0] == 100 || gExecutionLog[1] == 100);
    const bool has200 = (gExecutionLog[0] == 200 || gExecutionLog[1] == 200);
    ASSERT_TRUE(has100) << "GroupA (100) should have run";
    ASSERT_TRUE(has200) << "GroupB (200) should have run";
}

TEST(SchedulingTest, AfterConstraintEnforcesOrder)
{
    gExecutionLog.clear();

    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<AfterPlugin<Startup::Start, OrderedFirst, OrderedSecond>>();
    ecs.Start<Startup>();

    ASSERT_EQ(gExecutionLog.size(), 2);
    ASSERT_EQ(gExecutionLog[0], 1);
    ASSERT_EQ(gExecutionLog[1], 2);
}

TEST(SchedulingTest, BeforeConstraintEnforcesOrder)
{
    gExecutionLog.clear();

    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<BeforePlugin<Startup::Start, OrderedFirst, OrderedSecond>>();
    ecs.Start<Startup>();

    ASSERT_EQ(gExecutionLog.size(), 2);
    ASSERT_EQ(gExecutionLog[0], 1);
    ASSERT_EQ(gExecutionLog[1], 2);
}

TEST(SchedulingTest, SystemsInDifferentSchedulesBothRun)
{
    gExecutionLog.clear();

    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, StartupSystem>>()
        .AddPlugin<SingleSystemPlugin<Cel::TearDown::Middle, TearDownSystem>>();
    ecs.Start<Startup>().End<Cel::TearDown>();

    ASSERT_EQ(gExecutionLog.size(), 2);
    ASSERT_EQ(gExecutionLog[0], 1) << "Startup system should run first";
    ASSERT_EQ(gExecutionLog[1], 2) << "TearDown system should run second";
}

TEST(SchedulingTest, ScheduleSlotsRunInEnumOrder)
{
    gExecutionLog.clear();

    App ecs;

    // Add out of order
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<SingleSystemPlugin<Startup::Start, SlotMiddle>>()
        .AddPlugin<SingleSystemPlugin<Startup::Last, SlotLast>>()
        .AddPlugin<SingleSystemPlugin<Startup::First, SlotFirst>>();
    ecs.Start<Startup>();

    ASSERT_EQ(gExecutionLog.size(), 3);
    ASSERT_EQ(gExecutionLog[0], 1)
        << "Startup::First slot should run before Startup::Last";
    ASSERT_EQ(gExecutionLog[1], 2);
    ASSERT_EQ(gExecutionLog[2], 3);
}

}