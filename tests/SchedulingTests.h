#pragma once

#include "TestHelpers.h"

#include "core/App.h"

#include <gtest/gtest.h>
#include <vector>

using namespace Cel;

inline std::vector<int> gExecutionLog;

// Test a single system works
inline void
SingleSystem(Resource<World>&)
{
    gExecutionLog.push_back(1);
}

// Test chains order correctly
inline void
ChainFirst(Resource<World>&)
{
    gExecutionLog.push_back(10);
}

inline void
ChainSecond(Resource<World>&)
{
    gExecutionLog.push_back(20);
}

// Groups can run parallel, ensure both run
inline void
GroupA(Resource<World>&)
{
    gExecutionLog.push_back(100);
}

inline void
GroupB(Resource<World>&)
{
    gExecutionLog.push_back(200);
}

// Ensure systems are scheduled correctly relative to each other
inline void
OrderedFirst(Resource<World>&)
{
    gExecutionLog.push_back(1);
}

inline void
OrderedSecond(Resource<World>&)
{
    // Must run after OrderedFirst
    ASSERT_FALSE(gExecutionLog.empty())
        << "OrderedFirst should have run before OrderedSecond";
    ASSERT_EQ(gExecutionLog.back(), 1)
        << "OrderedFirst should be immediately before OrderedSecond";
    gExecutionLog.push_back(2);
}

inline void
StartupSystem(Resource<World>&)
{
    gExecutionLog.push_back(1);
}

inline void
TearDownSystem(Resource<World>&)
{
    gExecutionLog.push_back(2);
}

// Ensure schedule enum runs in correct order
inline void
SlotFirst(Resource<World>&)
{
    gExecutionLog.push_back(1);
}

inline void
SlotMiddle(Resource<World>&)
{
    gExecutionLog.push_back(2);
}

inline void
SlotLast(Resource<World>&)
{
    gExecutionLog.push_back(3);
}
