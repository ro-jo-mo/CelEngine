#include "renderer/render-graph/RenderGraphPlugin.h"

#include "renderer/Queues.h"
#include "renderer/VulkanSetup.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/render-graph/RenderGraph.h"

using namespace Cel;
using namespace Cel::Renderer;

static void
execute_graph(Resource<RenderGraph::Graph>& graph,
              ParallelResource<RenderGraph::PassServer>& passServer,
              Resource<Swapchain>& swapchain,
              Resource<VulkanResourceManager>& manager)
{
    auto access = passServer.absolute();

    graph->execute(*access, *swapchain, *manager);
    graph->reset();
}

void
RenderGraph::RenderGraphPlugin::compile_graph(
    Resource<Graph>& graph,
    Resource<VulkanResourceManager>& manager,
    Resource<RenderExtent>& extent,
    Resource<Asset::SharedAssetData>& shared,
    ParallelResource<PassServer>& passServer)
{
    auto access = passServer.absolute();

    // We release the resources first, so that the manager can see what's
    // actually available during graph tr
    access->release_resources(*manager);

    graph->compile(*manager);

    access->update_frame(extent->extent,
                         graph->passes,
                         graph->bufferHandleToMapped,
                         graph->imageHandleToMapped,
                         graph->perFrameBuffers,
                         graph->perFrameImages,
                         graph->setupPasses,
                         *manager,
                         *shared);
}

void
RenderGraph::RenderGraphPlugin::acquire_swapchain_image(
    Resource<Swapchain>& swapchain,
    ParallelResource<PassServer>& passServer)
{
    passServer.absolute()->acquire_swapchain_image(*swapchain);
}

static void
init_pass_server(Resource<VulkanContext>& context,
                 ParallelResource<RenderGraph::PassServer>& server)
{
    std::array queues = { Queues::graphics, Queues::compute, Queues::transfer };

    server.initialise(context->device, queues);
}

void
RenderGraph::RenderGraphPlugin::build(PluginManager& manager)
{
    manager.insert_parallel_resource_as_null<PassServer>();
    manager.insert_resource<Graph>();

    manager.add_system(Startup::PreStart, init_pass_server)
        .after(initialise_renderer);

    manager.add_system(Render::Update, compile_graph);
    // Acquire after compile_graph has waited on this frame's fence, so the
    // acquire semaphore is free to reuse. Must precede present pass recording
    manager.add_system(Render::Update, acquire_swapchain_image)
        .after(compile_graph);
    manager.add_system(Render::Last, execute_graph);
}
