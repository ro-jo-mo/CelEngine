#pragma once
#include "AssetServer.h"
#include "DeletionQueue.h"
#include "VulkanTypes.h"
#include "ecs/System.h"

namespace Cel::Renderer {

void
cleanup_renderer(Resource<FinalCleanup>& cleanup,
                Resource<FrameData>& frameData,
                Resource<VulkanContext>& context);

void
cleanup_asset_server(Resource<AssetServer>& assetServer);

}