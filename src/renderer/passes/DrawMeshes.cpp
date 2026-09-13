#include "renderer/passes/DrawMeshes.h"

#include "renderer/AssetServer.h"
#include "renderer/SceneData.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/render-graph/RenderGraph.h"
#include "renderer/resource-management/PipelineBuilder.h"

namespace {

const auto indirectDataPass =
    Cel::Renderer::Passes::HandleAllocator::allocate_pass(
        "mesh_draw_indirect_pass");

const auto indirectStagingBuffer =
    Cel::Renderer::Passes::HandleAllocator::allocate_buffer(
        "mesh_draw_indirect_staging_buffer");

const auto indirectBuffer =
    Cel::Renderer::Passes::HandleAllocator::allocate_buffer(
        "mesh_draw_indirect_buffer");

}

void
Cel::Renderer::Passes::register_indirect_draw_data_pass(
    Resource<RenderGraph::Graph>& graph)
{
    auto pass = RenderGraph::PassBuilder(indirectDataPass);

    pass.create_buffer(indirectStagingBuffer,
                       true,
                       MAX_ENTITIES * sizeof(VkDrawIndexedIndirectCommand),
                       VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
                       VMA_MEMORY_USAGE_CPU_TO_GPU)
        .create_buffer(indirectBuffer,
                       true,
                       MAX_ENTITIES * sizeof(VkDrawIndexedIndirectCommand),
                       VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_2_INDIRECT_BUFFER_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY)
        .upload_buffer(indirectStagingBuffer, indirectBuffer);

    // All indirect data needs to be created before scene data so the entity
    // buffer is correctly made
    graph->add_setup_pass(pass.build()).before(Passes::createSceneDataPass);
}

void
Cel::Renderer::Passes::create_indirect_draw_data(
    Query<With<Entity, Handle<Assets::Mesh>, Handle<Assets::Material>>>&
        renderables,
    ParallelResource<RenderGraph::PassServer>& passServer,
    Resource<Assets::AssetServer>& assetServer,
    Resource<SceneData>& scene)
{
    std::vector<VkDrawIndexedIndirectCommand> indirectCalls;

    indirectCalls.reserve(renderables.size());

    for (const auto& [entity, transform, meshHandle, matHandle] : renderables) {

        auto index = scene->get_entity_index(entity);

        auto mesh = assetServer->get_mesh(meshHandle);

        VkDrawIndexedIndirectCommand call{
            .indexCount = mesh.indexCount,
            .instanceCount = 1,
            .firstIndex = mesh.firstIndex,
            .vertexOffset = mesh.vertexOffset,
            // Mark first instance as the index into entity data
            .firstInstance = index
        };

        indirectCalls.push_back(call);
    }

    // copy into buffer
    // We want all this data to be gpu local
    // We could simply create a large staging buffer and reuse it for each
    // upload, which might perform better
    // Alternatively an upload queue might be
    // beneficial as well

    VkCommandBuffer cmd;
    {
        cmd = passServer.write()->get_cmd_buffer(indirectDataPass);
    }

    auto& access = passServer.illegal();

    Utils::upload_to_buffer(cmd,
                            indirectCalls.data(),
                            sizeof(VkDrawIndexedIndirectCommand),
                            access.get_resource(indirectBuffer),
                            0,
                            access.get_resource(indirectStagingBuffer));
}

void
Cel::Renderer::Passes::register_draw_mesh(Resource<Assets::AssetServer>& server,
                                          Resource<RenderGraph::Graph>& graph)
{
    auto pass = RenderGraph::PassBuilder(drawMeshPass);

    pass.write_image(Passes::drawImage,
                     VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                     VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        .write_image(Passes::depthImage,
                     VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
        .read_buffer(indirectBuffer,
                     VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
                     VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT);

    server->declare_scene_access(pass);

    graph->add_pass(pass.build());
}

void
Cel::Renderer::Passes::draw_mesh(
    Query<With<Entity, Handle<Assets::Mesh>, Handle<Assets::Material>>>&
        renderables,
    ParallelResource<RenderGraph::PassServer>& passServer)
{
    // When should I create indirect data? I'll do it in another pass so we can
    // use auto sync

    VkCommandBuffer cmd;
    {
        cmd = passServer.write()->get_cmd_buffer(drawMeshPass);
    }

    auto access = passServer.illegal();

    // Rendering info setup
    VkRenderingAttachmentInfo colourAttachment = Initialisers::attachment_info(
        access.get_resource(Passes::drawImage).imageView,
        nullptr,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkRenderingAttachmentInfo depthAttachment =
        Initialisers::depth_attachment_info(
            access.get_resource(Passes::depthImage).imageView,
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo renderInfo = Initialisers::rendering_info(
        access.get_extent(), &colourAttachment, &depthAttachment);

    vkCmdBeginRendering(cmd, &renderInfo);

    static auto pipeline = PipelineBuilder(access.get_device())
                               .add_shader_module("./shaders/mesh.vert.spv",
                                                  VK_SHADER_STAGE_VERTEX_BIT)
                               .add_shader_module("./shaders/mesh.frag.spv",
                                                  VK_SHADER_STAGE_FRAGMENT_BIT)
                               .build();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    const auto& indirect = access.get_resource(indirectBuffer);

    // Lastly execute indirect draw
    vkCmdDrawIndexedIndirect(cmd,
                             indirect.buffer,
                             0,
                             renderables.size(),
                             sizeof(VkDrawIndexedIndirectCommand));
}
