#include "renderer/passes/SetDescriptorsAndSceneData.h"

#include "renderer/AssetServer.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/RenderGraph.h"

void
Cel::Renderer::Passes::register_bind_descriptors_pass(
    Query<With<Handle<Assets::Mesh>, Handle<Assets::Material>>> toDraw,
    Resource<Assets::AssetServer>& server,
    Resource<RenderGraph::Graph>& graph)
{
    auto pass = RenderGraph::PassBuilder(Passes::bindDescriptorsPass);

    server->declare_scene_access(pass);

    // Per entity data, scene data ...
    // The real problem here is I don't want to have to declare access to all of
    // these every time... Same access as asset server?
    pass.create_buffer();
    pass.write_buffer();

    graph->add_pass(pass.build()).after(Passes::uploadAssetsPass);
}

void
Cel::Renderer::Passes::bind_descriptors(
    Resource<Assets::AssetServer>& assetServer)
{
}
