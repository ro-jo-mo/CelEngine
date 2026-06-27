#pragma once

#include "TestHelpers.h"

#include "core/App.h"

#include <gtest/gtest.h>

using namespace Cel;

inline void
ReadIntResource(Resource<IntResource>& res)
{
    ASSERT_EQ(res->value, 42);
}

inline void
MutateIntResource(Resource<IntResource>& res)
{
    res->value += 8;
}

inline void
VerifyMutatedIntResource(Resource<IntResource>& res)
{
    ASSERT_EQ(res->value, 50);
}
