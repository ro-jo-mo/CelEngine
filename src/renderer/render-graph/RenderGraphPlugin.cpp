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
    ParallelResource<PassServer>& server)
{
    graph->compile(*manager);

    server.absolute()->update_frame(extent->extent,
                                    graph->passes,
                                    graph->bufferHandleToMapped,
                                    graph->imageHandleToMapped,
                                    graph->perFrameBuffers,
                                    graph->perFrameImages,
                                    *manager);
}

static void
init_pass_server(Resource<VulkanContext>& context,
                 ParallelResource<RenderGraph::PassServer>& server)
{
    std::array queues = { Queues::graphics, Queues::compute, Queues::transfer };

    server.initialise(context->device, queues);
}

void
RenderGraph::RenderGraphPlugin::build(SystemScheduler scheduler,
                                      ResourceManager& resourceManager)
{
    resourceManager.insert_parallel_resource_as_null<PassServer>();
    resourceManager.insert_resource<Graph>();

    scheduler.add_system(Startup::PreStart, init_pass_server)
        .after(initialise_renderer);

    scheduler.add_system(Render::Update, compile_graph);
    scheduler.add_system(Render::Last, execute_graph);
}
