#pragma once
#include "DeletionQueue.h"
#include "VulkanTypes.h"
#include "ecs/System.h"

namespace Cel::Renderer {
class CleanupRenderer final
    : public System<Resource<FinalCleanup>, Resource<VulkanContext>>
{
  public:
    void Run(Resource<FinalCleanup>& cleanup,
             Resource<VulkanContext>& context) override;
};
}