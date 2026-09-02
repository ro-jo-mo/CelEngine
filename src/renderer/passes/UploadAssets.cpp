#include "renderer/passes/UploadAssets.h"

#include "renderer/AssetServer.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/render-graph/RenderGraph.h"

void
Cel::Renderer::Passes::register_asset_upload_pass(
    Resource<Assets::AssetServer>& server,
    Resource<VulkanResourceManager>& manager,
    Resource<RenderGraph::Graph>& graph)
{
    auto pass = RenderGraph::PassBuilder(uploadAssetsPass);

    server->register_pass(pass, manager);

    graph->add_pass(pass.build());
}

void
Cel::Renderer::Passes::upload_assets(
    Resource<Assets::AssetServer>& assetServer,
    ParallelResource<RenderGraph::PassServer> passServer)
{
    assetServer->flush(passServer);
}

// Rework so we do in fact distinguish between the per frame and reused
// resources I really want handles for things like shadowTex and such Where
// they're not per frame, but still ideally defined in Passes:: I should be able
// to tag the Buffer/ImageCreate structs and build off that fairly easily
// Should I move towards a unified handle allocator?
// Cons -> Thread safety, lots of locking
// This could let us distinguish for reused across frame resources
// i.e. VulkanResourceManager->does_exist(handle) if not create
// Whereas per frame would always create
// Where do  I need to change?
// PassBuilder : obvious
// PassServer : freeing of resources and such
// RenderGraph : resource mapping, null / default state
// Resource manager : does this exist?

// Do I actually want these handles to directly map? Probably not, if for
// example we change shadow resolution, it would complicate things massively
// Instead I'll map them still. We'll have to free them before we acquire them
// again, and look for exact matches only when aliasing

// Funky rule? per frame resources should use a looser aliasing requirements
// whereas transient resources should be strict