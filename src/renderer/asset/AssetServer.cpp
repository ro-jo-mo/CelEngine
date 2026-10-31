#include "renderer/asset/AssetServer.h"

#include "core/Error.h"
#include "renderer/VulkanUtils.h"
#include "renderer/passes/HandleAllocator.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/resource-management/VulkanResourceManager.h"

using namespace Cel::Renderer::Asset;
using namespace Cel::Renderer;
using namespace Cel;

namespace {
inline const auto vertexStagingBuffer =
    Passes::HandleAllocator::allocate_buffer("vertex_staging_buffer");
inline const auto indexStagingBuffer =
    Passes::HandleAllocator::allocate_buffer("index_staging_buffer");
inline const auto materialStagingBuffer =
    Passes::HandleAllocator::allocate_buffer("material_staging_buffer");

// Set on primitive handles owned by the procedural loader. Loaders only see
// the masked 0..n index
constexpr uint32_t PRIMITIVE_MASK = 1u << 31;
}

AssetServer::AssetServer(VulkanResourceManager& manager,
                         Resource<SharedAssetData>& shared)
    : gltfLoader(manager.device)
    , imageLoader(manager.device)
{

    SharedAssetData data{
        .device = manager.device,
        .vertexBuffer =
            MegaBuffer({ .allocSize = 2 << 16,
                         .usages = VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT |
                                   VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                                   VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                                   VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                         .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY },
                       "vertex_buffer",
                       manager),
        .indexBuffer =
            MegaBuffer({ .allocSize = 2 << 16,
                         .usages = VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
                                   VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                                   VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                                   VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                         .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY },
                       "index_buffer",
                       manager),
        .materialBuffer =
            MegaBuffer({ .allocSize = 2 << 16,
                         .usages = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                                   VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                                   VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                         .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY },
                       "material_buffer",
                       manager)
    };

    shared.initialise(data);

    vertexBufferHandle = shared->vertexBuffer.handle;
    indexBufferHandle = shared->indexBuffer.handle;
    materialBufferHandle = shared->materialBuffer.handle;
}

Handle<SceneAsset>
AssetServer::load_gltf(std::string file, GltfLabel::Label label)
{
    return gltfLoader.load_gltf(file, label);
}

Handle<Primitive>
AssetServer::create_cube(glm::vec3 size)
{
    auto handle = proceduralLoader.create_cube(size);
    handle.index |= PRIMITIVE_MASK;
    return handle;
}

Primitive&
AssetServer::get_primitive(Handle<Primitive> handle)
{
    if (handle.index & PRIMITIVE_MASK) {
        handle.index &= ~PRIMITIVE_MASK;
        return proceduralLoader.get_primitive(handle);
    }
    return gltfLoader.get_primitive(handle);
}

Handle<ImageAsset>
AssetServer::load_image(std::string file)
{
    return imageLoader.load_image(file);
}

ImageAsset&
AssetServer::get_image(Handle<ImageAsset> handle)
{
    return imageLoader.get_image(handle);
}

Handle<TextureAsset>
AssetServer::load_texture(std::string file, SamplerSettings sampler)
{
    return imageLoader.load_texture(file, sampler);
}

uint32_t
AssetServer::get_texture(Handle<TextureAsset> handle)
{
    return imageLoader.get_texture_index(handle);
}

void
AssetServer::add_scene_to_entity(Entity entity, Handle<SceneAsset> handle)
{
    gltfLoader.add_scene_to_entity(entity, handle);
}

void
AssetServer::flush_cpu(VulkanResourceManager& manager,
                       World& world,
                       SharedAssetData& shared,
                       RenderGraph::PassBuilder& pass)
{
    gltfLoader.flush_cpu(manager, world, shared, pass);
    imageLoader.flush_cpu(manager, shared, pass);
    proceduralLoader.flush_cpu(shared);

    shared.vertexBuffer.register_upload(vertexStagingBuffer, pass);
    shared.indexBuffer.register_upload(indexStagingBuffer, pass);
    shared.materialBuffer.register_upload(materialStagingBuffer, pass);
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

    shared.vertexBuffer.upload_to_gpu(cmd, vertexStagingBuffer, *access);
    shared.indexBuffer.upload_to_gpu(cmd, indexStagingBuffer, *access);
    shared.materialBuffer.upload_to_gpu(cmd, materialStagingBuffer, *access);

    gltfLoader.flush_gpu(cmd, *access);
    imageLoader.flush_gpu(cmd, *access);
}

void
AssetServer::declare_scene_access(RenderGraph::PassBuilder& pass)
{
    pass.read_buffer(vertexBufferHandle,
                     VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                     VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT);
    pass.read_buffer(indexBufferHandle,
                     VK_ACCESS_2_INDEX_READ_BIT,
                     VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT);
    pass.read_buffer(materialBufferHandle,
                     VK_ACCESS_2_SHADER_READ_BIT,
                     VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

    gltfLoader.declare_scene_access(pass);
    imageLoader.declare_image_access(pass);
}

void
AssetServer::cleanup()
{
    gltfLoader.cleanup();
    imageLoader.cleanup();
}
