#pragma once

#include "ecs/Types.h"
#include <unordered_set>

namespace Cel {
struct Children
{
    std::unordered_set<Entity> children;
};

struct Parent
{
    Entity parent;
};
}
