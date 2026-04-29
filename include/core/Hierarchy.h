#pragma once
#include <vector>

#include "ecs/Types.h"

namespace Cel {
    struct Parent {
        std::vector<Entity> children;
    };

    struct Child {
        Entity parent;
    };
}
