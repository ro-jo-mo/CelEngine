#pragma once
#include "VulkanTypes.h"
#include "asset/AssetServer.h"
#include "ecs/System.h"
#include "resource-management/DeletionQueue.h"

namespace Cel::Renderer {

void
cleanup_renderer(Resource<FinalCleanup>& cleanup,
                 Resource<VulkanContext>& context);

void
cleanup_asset_server(Resource<Asset::AssetServer>& assetServer);

}