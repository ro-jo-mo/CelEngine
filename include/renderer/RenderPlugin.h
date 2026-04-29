#pragma once
#include "ecs/Plugin.h"

namespace Cel {
    class RenderPlugin final : public Plugin {
    public:
        void Build(Scheduler scheduler, ResourceManager &resourceManager) override;
    };
}
