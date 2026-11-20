#pragma once
#include "../RenderTypes.h"
#include "../asset/AssetServer.h"
#include "DeletionQueue.h"
#include "ecs/System.h"

namespace Cel::Renderer {

void
cleanup_renderer(Resource<FinalCleanup>& cleanup,
                 Resource<VulkanContext>& context);

void
cleanup_asset_server(Resource<Asset::AssetServer>& assetServer,
                     Resource<Asset::SharedAssetData>& shared);

}