#pragma once
#include "common/Handle.h"
#include "ecs/Query.h"
#include "ecs/Resource.h"

namespace Cel {
struct GlobalTransform;
}
namespace Cel::Renderer {
class Camera;
}
namespace Cel::Renderer::RenderGraph {
class Graph;
}
namespace Cel::Renderer::Assets {
struct Material;
class AssetServer;
}

namespace Cel::Renderer::Passes {

struct SceneData;

// The key thing here is that we'll only create entity data for entities added
// to the SceneData resource
// This requires that all indirect draw commands are created before this
// This is likely to create a weird set of dependencies, where passes that rely
// on the gpu data and are executed after this pass, are actually recorded
// before this pass (on the cpu)

void
register_create_scene_data_pass(
    Query<With<Entity, Handle<Assets::Material>, GlobalTransform>>& entities,
    Query<With<Camera, GlobalTransform>>& camera,
    Resource<SceneData>& sceneData,
    Resource<Assets::AssetServer>& server,
    Resource<RenderGraph::Graph>& graph);

void
create_scene_data();

}