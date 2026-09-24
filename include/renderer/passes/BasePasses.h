#pragma once
#include "common/Handle.h"
#include "ecs/Query.h"
#include "ecs/Resource.h"

namespace Cel {
namespace Renderer {
struct RenderExtent;
class VulkanResourceManager;
class Camera;
}
struct GlobalTransform;
}
namespace Cel::Renderer::Assets {
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

    static void create_and_bind_scene_data(
        Query<With<Handle<Assets::Material>, GlobalTransform>>& entities,
        Query<With<Camera, GlobalTransform>>& camera,
        Resource<SceneData>& sceneData,
        Resource<Assets::AssetServer>& server,
        ParallelResource<RenderGraph::PassServer>& passServer);

    static void register_asset_upload_pass(
        Resource<Assets::AssetServer>& server,
        Resource<VulkanResourceManager>& manager,
        Resource<RenderGraph::Graph>& graph);

    static void upload_assets(
        Resource<Assets::AssetServer>& assetServer,
        ParallelResource<RenderGraph::PassServer>& passServer);

};

}