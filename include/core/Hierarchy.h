#pragma once

#include "ecs/Types.h"
#include <unordered_set>

namespace Cel {
struct Parent
{
    std::unordered_set<Entity> children;
};

struct Child
{
    Entity parent;
};
}
