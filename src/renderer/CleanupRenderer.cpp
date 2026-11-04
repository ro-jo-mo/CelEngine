#include "renderer/CleanupRenderer.h"
#include "renderer/RenderTypes.h"

void
Cel::Renderer::cleanup_renderer(Resource<FinalCleanup>& cleanup,
                                Resource<VulkanContext>& context)
{
    vkDeviceWaitIdle(context->device);
    
    cleanup->flush();
}

void
Cel::Renderer::cleanup_asset_server(Resource<Asset::AssetServer>& assetServer,
                                    Resource<Asset::SharedAssetData>& shared)
{
    // Runs before cleanup_renderer, so the gpu may still be using these
    vkDeviceWaitIdle(shared->device);

    assetServer->cleanup();
    shared->samplers.cleanup();
}