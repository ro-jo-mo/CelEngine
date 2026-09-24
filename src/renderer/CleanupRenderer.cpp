#include "renderer/CleanupRenderer.h"
#include "renderer/VulkanTypes.h"

void
Cel::Renderer::cleanup_renderer(Resource<FinalCleanup>& cleanup,
                                Resource<VulkanContext>& context)
{
    vkDeviceWaitIdle(context->device);
    
    cleanup->flush();
}

void
Cel::Renderer::cleanup_asset_server(Resource<Assets::AssetServer>& assetServer)
{
    assetServer->cleanup();
}