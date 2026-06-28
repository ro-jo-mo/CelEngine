#pragma once
#include "AssetServer.h"
#include "DeletionQueue.h"
#include "VulkanTypes.h"
#include "ecs/System.h"

namespace Cel::Renderer {

void
CleanupRenderer(Resource<FinalCleanup>& cleanup,
                Resource<FrameData>& frameData,
                Resource<VulkanContext>& context);

void
CleanupAssetServer(Resource<AssetServer>& assetServer);

}