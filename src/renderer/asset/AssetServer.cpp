#define STB_IMAGE_IMPLEMENTATION

#include "renderer/asset/AssetServer.h"

#include "core/Error.h"
#include "renderer/VulkanUtils.h"
#include "renderer/passes/HandleAllocator.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/glm.hpp>
#include <ktx.h>
#include <ranges>
#include <stb_image.h>

using namespace Cel::Renderer::Asset;
using namespace Cel::Renderer;
using namespace Cel;

namespace {
inline const auto vertexStagingBuffer =
    Passes::HandleAllocator::allocate_buffer("vertex_staging_buffer");
inline const auto indiceStagingBuffer =
    Passes::HandleAllocator::allocate_buffer("indice_staging_buffer");
inline const auto materialStagingBuffer =
    Passes::HandleAllocator::allocate_buffer("material_staging_buffer");
}

void
AssetServer::flush_cpu(VulkanResourceManager& manager,
                       SharedAssetData& shared,
                       RenderGraph::PassBuilder& pass)
{
    gltfLoader.flush_cpu(manager, shared, pass);
}

void
AssetServer::flush_gpu(SharedAssetData& shared,
                       ParallelResource<RenderGraph::PassServer>& server)
{
    VkCommandBuffer cmd;
    {
        cmd = server.write()->get_cmd_buffer(Passes::uploadAssetsPass);
    }

    auto access = server.partial_read();

    shared.vertexBuffer.push_to_gpu(cmd,
                                    access->get_resource(vertexStagingBuffer));
    shared.indexBuffer.push_to_gpu(cmd,
                                   access->get_resource(indiceStagingBuffer));
    shared.materialBuffer.push_to_gpu(
        cmd, access->get_resource(materialStagingBuffer));

    gltfLoader.flush_gpu(cmd, *access);
}
