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
Cel::Renderer::cleanup_asset_server(Resource<Asset::AssetServer>& assetServer)
{
    assetServer->cleanup();
}