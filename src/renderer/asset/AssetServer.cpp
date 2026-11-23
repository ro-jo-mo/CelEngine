#include <utility>

#include "renderer/asset/AssetServer.h"

#include "../../../include/renderer/vulkan/VulkanUtils.h"
#include "core/Error.h"
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

// Tag this bit to show a handle is from the procedural loader
constexpr uint32_t PROCEDURAL_TAG = 1u << 31;
}

AssetServer::AssetServer(VulkanResourceManager& manager,
                         Resource<SharedAssetData>& shared)
    : gltfLoader(manager.device)
    , imageLoader(manager.device)
{
    SharedAssetData data{
        .device = manager.device,
        .vertexBuffer =
            MegaBuffer({ .allocSize = VERTEX_BUFFER_SIZE,
                         .usages = VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT |
                                   VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                                   VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                                   VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                         .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY },
                       "vertex_buffer",
                       manager),
        .indexBuffer =
            MegaBuffer({ .allocSize = INDEX_BUFFER_SIZE,
                         .usages = VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
                                   VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                                   VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                                   VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                         .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY },
                       "index_buffer",
                       manager),
        .materialBuffer =
            MegaBuffer({ .allocSize = MATERIAL_BUFFER_SIZE,
                         .usages = VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                                   VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                                   VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                         .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY },
                       "material_buffer",
                       manager),
        .samplers = SamplerCache(manager.device)
    };

    shared.initialise(data);

    vertexBufferHandle = shared->vertexBuffer.handle;
    indexBufferHandle = shared->indexBuffer.handle;
    materialBufferHandle = shared->materialBuffer.handle;

    // Create default values
    assert(shared->samplers.add_sampler(SamplerSettings::Linear) ==
           Defaults::LinearSampler);
    assert(shared->samplers.add_sampler(SamplerSettings::Pixel) ==
           Defaults::PixelSampler);

    constexpr MaterialConstants defaultConstants = {
        .baseColorFactors = { 1, 1, 1, 1 },
        .metalRoughnessFactors = { 1, 1, 1, 1 },
        .colorTextureIndex = Defaults::WhiteTexture,
        .metalRoughnessTextureIndex = Defaults::WhiteTexture,
        .normalTextureIndex = Defaults::NormalTexture,
        .colorSamplerIndex = Defaults::LinearSampler,
        .metalRoughnessSamplerIndex = Defaults::LinearSampler,
        .normalSamplerIndex = Defaults::LinearSampler
    };

    assert(shared->materialBuffer.allocate(&defaultConstants,
                                           sizeof(MaterialConstants)) ==
           Defaults::Material);

    auto create_img = [&](auto col) {
        const std::vector<uint32_t> pixels{ col };

        // Create as UNORM.
        // Suitable only for non colour textures, or a full white / black
        // texture used for colour
        return create_image(pixels, VK_FORMAT_R8G8B8A8_UNORM, { 1, 1, 1 });
    };

    assert((create_img(glm::packUnorm4x8(glm::vec4(1, 1, 1, 1))).index &
            ~PROCEDURAL_TAG) == Defaults::WhiteTexture);

    assert((create_img(glm::packUnorm4x8(glm::vec4(0.5, 0.5, 1, 1))).index &
            ~PROCEDURAL_TAG) == Defaults::NormalTexture);

    {
        const uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        const uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));

        std::vector<uint32_t> pixels;
        pixels.resize(16 * 16);
        // for 16x16 checkerboard texture
        for (int x = 0; x < 16; x++) {
            for (int y = 0; y < 16; y++) {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }

        // Create as SRGB. It's unlikely to be used as any way other than a
        // colour texture
        assert((create_image(pixels, VK_FORMAT_R8G8B8A8_UNORM, { 16, 16, 1 })
                    .index &
                ~PROCEDURAL_TAG) == Defaults::MagentaCheckerboard);
    }
}

Handle<SceneAsset>
AssetServer::load_gltf(std::string file, GltfLabel::Label label)
{
    return gltfLoader.load_gltf(std::move(file), label);
}

Handle<Primitive>
AssetServer::create_cube(const glm::vec3 size)
{
    return { proceduralLoader.create_cube(size).index | PROCEDURAL_TAG };
}
Handle<ImageAsset>
AssetServer::create_image(const std::vector<uint32_t>& pixels,
                          const VkFormat format,
                          const VkExtent3D extent)
{
    return { proceduralLoader.create_image(pixels, format, extent).index |
             PROCEDURAL_TAG };
}

Primitive&
AssetServer::get_primitive(Handle<Primitive> handle)
{
    if (handle.index & PROCEDURAL_TAG) {
        return proceduralLoader.get_primitive(
            { handle.index & ~PROCEDURAL_TAG });
    }
    return gltfLoader.get_primitive(handle);
}

Handle<ImageAsset>
AssetServer::load_image(const std::string& file)
{
    return imageLoader.load_image(file);
}

ImageAsset&
AssetServer::get_image(Handle<ImageAsset> handle)
{
    if (handle.index & PROCEDURAL_TAG) {
        return proceduralLoader.get_image({ handle.index & ~PROCEDURAL_TAG });
    }

    return imageLoader.get_image(handle);
}

Handle<TextureAsset>
AssetServer::load_texture(const std::string& file, SamplerSettings sampler)
{
    return imageLoader.load_texture(file, sampler);
}

TextureAsset
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
    // We MUST flush the procedural loader first to ensure setup data comes
    // first
    proceduralLoader.flush_cpu(manager, shared, pass);

    gltfLoader.flush_cpu(manager, world, shared, pass);
    imageLoader.flush_cpu(manager, shared, pass);

    shared.vertexBuffer.register_upload_to_pass(vertexStagingBuffer, pass);
    shared.indexBuffer.register_upload_to_pass(indexStagingBuffer, pass);
    shared.materialBuffer.register_upload_to_pass(materialStagingBuffer, pass);
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

    shared.vertexBuffer.record_upload_cmd(cmd, vertexStagingBuffer, *access);
    shared.indexBuffer.record_upload_cmd(cmd, indexStagingBuffer, *access);
    shared.materialBuffer.record_upload_cmd(
        cmd, materialStagingBuffer, *access);

    proceduralLoader.flush_gpu(cmd, *access);
    gltfLoader.flush_gpu(cmd, *access);
    imageLoader.flush_gpu(cmd, *access);
}

void
AssetServer::declare_scene_access(RenderGraph::PassBuilder& pass)
{

    pass.read_buffer(vertexBufferHandle,
                     VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                     VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT)
        .read_buffer(indexBufferHandle,
                     VK_ACCESS_2_INDEX_READ_BIT,
                     VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT)
        .read_buffer(materialBufferHandle,
                     VK_ACCESS_2_SHADER_READ_BIT,
                     VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT)
        .read_buffer(Passes::sceneDataBuffer,
                     VK_ACCESS_2_SHADER_READ_BIT,
                     VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT)
        .read_buffer(Passes::entityDataBuffer,
                     VK_ACCESS_2_SHADER_READ_BIT,
                     VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT)
        .read_buffer(Passes::lightDataBuffer,
                     VK_ACCESS_2_SHADER_READ_BIT,
                     VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

    // TODO: Ideally instead of declaring access to every image, we'd only
    // declare access to the ones created this frame.
    // That way the render graph would only need to check transitions for those
    gltfLoader.declare_scene_access(pass);
    imageLoader.declare_image_access(pass);
}

void
AssetServer::cleanup()
{
    gltfLoader.cleanup();
    imageLoader.cleanup();
}
