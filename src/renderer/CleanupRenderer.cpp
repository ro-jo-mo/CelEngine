#include "renderer/CleanupRenderer.h"

void
Cel::Renderer::CleanupRenderer(Resource<FinalCleanup>& cleanup,
                               Resource<VulkanContext>& context)
{
    vkDeviceWaitIdle(context->device);
    cleanup->Flush();
}

void
Cel::Renderer::CleanupAssetServer(Resource<AssetServer>& assetServer,
                                  Resource<VulkanContext>& context)
{
    assetServer->Cleanup();
}
void
Cel::Renderer::CleanupAfterDraw(Resource<CurrentFrameData>& frameData,
                                Resource<VulkanContext>& context)
{
    frameData->Get().toDelete.Flush();
    frameData->Update();
}
