#pragma once

#include "../core/World.h"
#include "Descriptors.h"
#include "VulkanTypes.h"
#include "common/Handle.h"
#include "ecs/Types.h"
#include "renderer/AssetTypes.h"
#include "resource-management/MegaBuffer.h"

namespace Cel::Renderer::RenderGraph {
class PassServer;
class Graph;
class PassBuilder;
struct RenderPass;
}
// Pre declarations
namespace Cel::Renderer {
namespace Assets {
class AssetServer;
}
struct DrawData;
void
cleanup_asset_server(Resource<Assets::AssetServer>& assetServer);
}

namespace Cel::Renderer::Passes {

void
register_asset_upload_pass(Resource<Assets::AssetServer>& server,
                           Resource<VulkanResourceManager>& manager,
                           Resource<RenderGraph::Graph>& graph);
void
upload_assets(Resource<Assets::AssetServer>& assetServer,
              ParallelResource<RenderGraph::PassServer> passServer);
}

namespace Cel::Renderer::Assets {

// Comments:

// Workflow:
// Get asset handle from server
// Then you can add this asset to an entity through a method that adds all child
// nodes as distinct child entities in the ecs.
// At a later point I can optimise this, baking parts of the gltf that don't
// need to be their own entity

// A caching mechanism should be used on several levels
// Firstly we should not load the same gltf file twice
// Keep a map of file paths to asset handle
// The same material and mesh may be used in the same model several times
// For example wheels on a car

// Asset unloading:
// Once all references to an asset are deleted, i.e. no entities exist with this
// asset handle It should be added to a deletion queue. While in the deletion
// queue, it should still be accessible through the asset cache
// If loaded again, take out of the deletion queue
// Otherwise, whenever we determine more memory is required
// flush the deletion queue (until we have enough memory? or until empty queue?)

class AssetServer
{
  public:
    explicit AssetServer(VulkanResourceManager& manager);

    Handle<AssetNode> load_gltf_asset(const char* filepath);

    void add_asset_to_entity(Entity entity,
                             Handle<AssetNode> handle,
                             Resource<World>& world) const;

    Handle<ImageAsset> load_ktx(const std::string& filepath);

    AllocatedImage& get_image_from_handle(Handle<ImageAsset> handle);

    /**
     * Registers that this render pass accesses loaded assets.
     * I make possibly problematic assumptions about the pipeline stages
     *
     * There is a big
     * caveat here, that the pass be must registered *after* the
     * "register_asset_upload_pass" system, as this is when the actual vulkan
     * handles manifest.
     *
     * Once async asset loading is introduced I'll likely fix
     * this.
     * @param pass
     */
    void declare_scene_access(RenderGraph::PassBuilder& pass);

    void declare_access_to_images(
        RenderGraph::PassBuilder& pass,
        const std::vector<Handle<ImageAsset>>& images);

  private:
    [[nodiscard]] Material get_material(Handle<Material> material) const;
    [[nodiscard]] Mesh get_mesh(Handle<Mesh> mesh) const;

    void create_defaults(VulkanResourceManager& manager);

    void load_image(fastgltf::Asset& asset, fastgltf::Image& gltfImage);

    void load_images(fastgltf::Asset& asset);
    void load_samplers(const fastgltf::Asset& asset);

    uint32_t resolve_texture_sampler(
        fastgltf::Asset& asset,
        const std::optional<fastgltf::TextureInfo>& textureInfo,
        size_t imageOffset,
        size_t samplerOffset);

    void load_materials(fastgltf::Asset& asset,
                        size_t imageOffset,
                        size_t samplerOffset);
    AssetNode load_nodes(fastgltf::Asset& asset, std::vector<Model>& models);
    std::vector<Model> load_models(fastgltf::Asset& asset,
                                   size_t materialOffset);

    void cleanup();

    void register_pass(RenderGraph::PassBuilder& pass,
                       Resource<VulkanResourceManager>& manager);

    void flush(ParallelResource<RenderGraph::PassServer>& passServer);

    std::unordered_map<const char*, Handle<AssetNode>> pathToAssetMap;

    // Assets, buffers, descriptors are coupled
    // These might be combined into a single struct later?
    std::vector<AssetNode> assets;
    std::vector<DescriptorAllocator> allocators;

    // Colour, RoughnessMetallic, Normal textures
    std::vector<AllocatedImage> gltfImages;
    // General purpose images
    std::vector<AllocatedImage> loadedImages;
    // Image samplers
    std::vector<VkSampler> samplers;

    // The CPU side data is purely for indexing into the mega buffers
    std::vector<Mesh> meshes;
    std::vector<Material> materials;

    // Staging buffers are a per frame resource (cpu to gpu)
    // As such we wish to leave lifetime entirely to the render graph
    // We reserve a healthy number of handles at startup
    std::vector<Handle<AllocatedBuffer>> reservedBufferHandles;

    // Commands
    struct CreateImgCmd
    {
        void* data;
        VkExtent3D extent;
        bool ktx;
        bool gltf;
    };
    struct CreateMeshCmd
    {};
    std::vector<CreateImgCmd> cmdCreateImgs;
    // Store image allocations here until they're officially created with data
    std::vector<AllocatedImage> uninitialisedImages;

    MegaBuffer vertexBuffer;
    MegaBuffer indiceBuffer;
    MegaBuffer materialBuffer;

    DescriptorWriter descriptorWriter;
    TextureCache textureCache;

    VkDevice device;

    // Beloved lack of module level access permissions. What a fantastic
    // language design!
    friend class Cel::Renderer::DrawData;
    friend void Cel::Renderer::cleanup_asset_server(
        Resource<AssetServer>& assetServer);
    friend void Cel::Renderer::Passes::register_asset_upload_pass(
        Resource<Assets::AssetServer>& server,
        Resource<VulkanResourceManager>& manager,
        Resource<RenderGraph::Graph>& graph);
    friend void Cel::Renderer::Passes::upload_assets(
        Resource<Assets::AssetServer>& assetServer,
        ParallelResource<RenderGraph::PassServer> passServer);
};
}
