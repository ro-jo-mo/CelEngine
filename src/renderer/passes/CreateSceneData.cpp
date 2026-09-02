#include "renderer/passes/CreateSceneData.h"

#include "core/Transform.h"
#include "renderer/Camera.h"
#include "renderer/SceneData.h"
#include "renderer/render-graph/PassBuilder.h"

#include <ranges>

void
Cel::Renderer::Passes::register_create_scene_data_pass(
    Query<With<Entity, Handle<Assets::Material>, GlobalTransform>>& entities,
    Query<With<Camera, GlobalTransform>>& camera,
    Resource<SceneData>& sceneData,
    Resource<Assets::AssetServer>& server,
    Resource<RenderGraph::Graph>& graph)
{
    const auto& [cam, camTrans] = *camera.begin();

    sceneData->data = { .verticesBufferAddress = 0,
                        .materialBufferAddress = 0,
                        .perEntityBufferAddress = 0,
                        .viewMatrix = cam.get_view_matrix(),
                        .projectionMatrix = cam.get_projection_matrix(),
                        .viewProjMatrix = cam.get_projection_matrix() *
                                          cam.get_view_matrix() };

    auto pass = RenderGraph::PassBuilder(Passes::createSceneDataPass);
}

void
Cel::Renderer::Passes::create_scene_data()
{
}
