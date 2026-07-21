#include "renderer/CleanupRenderer.h"
#include "renderer/VulkanTypes.h"

void
Cel::Renderer::cleanup_renderer(Resource<FinalCleanup>& cleanup,
                               Resource<FrameData>& frameData,
                               Resource<VulkanContext>& context)
{
    vkDeviceWaitIdle(context->device);

    for (auto& frame : frameData->frames) {
        frame.toDelete.flush();
    }
    cleanup->flush();
}

void
Cel::Renderer::cleanup_asset_server(Resource<AssetServer>& assetServer)
{
    assetServer->cleanup();
}