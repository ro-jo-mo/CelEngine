#pragma once
#include "common/Handle.h"
#include "core/App.h"
#include "ecs/Query.h"
#include "ecs/Resource.h"
#include "renderer/asset/AssetTypes.h"

namespace Cel {
namespace Renderer {
struct RenderExtent;
class VulkanResourceManager;
class Camera;
}
struct GlobalTransform;
}
namespace Cel::Renderer::Asset {
class AssetServer;
struct Material;
}
namespace Cel::Renderer::RenderGraph {
class PassServer;
class Graph;
}
namespace Cel::Renderer::Passes {
struct SceneData;

// Convenient friend
struct PassFriend
{

    static void register_present_pass(Resource<RenderGraph::Graph>& graph);

    static void register_create_scene_data_pass(
        Resource<RenderGraph::Graph>& graph);

    static void create_scene_data(
        Query<With<Handle<Asset::Material>, GlobalTransform>>& entities,
        Query<With<Camera, GlobalTransform>>& camera,
        Resource<SceneData>& sceneData,
        Resource<Asset::SharedAssetData>& shared,
        ParallelResource<RenderGraph::PassServer>& passServer);

    static void register_asset_upload_pass(
        Resource<Asset::AssetServer>& server,
        Resource<VulkanResourceManager>& manager,
        Resource<Asset::SharedAssetData>& shared,
        Resource<World>& world,
        Resource<RenderGraph::Graph>& graph);

    static void upload_assets(
        Resource<Asset::AssetServer>& assetServer,
        Resource<Asset::SharedAssetData>& shared,
        ParallelResource<RenderGraph::PassServer>& passServer);
};

}