#include "renderer/CleanupRenderer.h"
#include "renderer/VulkanTypes.h"

void
Cel::Renderer::CleanupRenderer(Resource<FinalCleanup>& cleanup,
                               Resource<FrameData>& frameData,
                               Resource<VulkanContext>& context)
{
    vkDeviceWaitIdle(context->device);

    for (auto& frame : frameData->frames) {
        frame.toDelete.Flush();
    }
    cleanup->Flush();
}

void
Cel::Renderer::CleanupAssetServer(Resource<AssetServer>& assetServer)
{
    assetServer->Cleanup();
}