#pragma once
#include "DeletionQueue.h"
#include "ecs/System.h"

namespace Cel::Renderer {
class CleanupRenderer final : public System<Resource<FinalCleanup>>
{
  public:
    void Run(Resource<FinalCleanup>&) override;
};
}