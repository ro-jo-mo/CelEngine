#include "renderer/CleanupRenderer.h"

void
Cel::Renderer::CleanupRenderer(Resource<FinalCleanup>& cleanup,
                               Resource<VulkanContext>& context)
{
    vkDeviceWaitIdle(context->device);
    cleanup->Flush();
}
