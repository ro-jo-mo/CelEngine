#pragma once

#include "../core/World.h"
#include "Descriptors.h"
#include "MegaBuffer.h"
#include "VulkanTypes.h"
#include "ecs/Types.h"
#include "renderer/AssetTypes.h"

namespace Cel::Renderer {

// Comments:
// If an object is made up of a hierarchy of models, like a scene might be I
// can't create this tree in the ECS. Existing in the ECS would implicitly mean
// it exists in the world. Additionally it would be difficult to structure

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
    AssetServer(Resource<VulkanContext>& context,
                Resource<VmaAllocator>& allocator,
                Resource<ImmediateSubmit>& immediate,
                Resource<GraphicsQueue>& graphicsQueue,
                Resource<GlobalDescriptorData>& globalDescriptorData)
        : verticeBuffer(2 << 16,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        VMA_MEMORY_USAGE_GPU_ONLY,
                        "vertice_mega_buffer_alloc",
                        *allocator)
        , indiceBuffer(2 << 16,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                           VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY,
                       "indice_mega_buffer_alloc",
                       *allocator)
        , materialBuffer(2 << 16,
                         VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                         VMA_MEMORY_USAGE_GPU_ONLY,
                         "material_mega_buffer_alloc",
                         *allocator)
        , context(*context)
        , allocator(*allocator)
        , immediate(*immediate)
        , graphicsQueue(*graphicsQueue)
        , globalDescriptorData(*globalDescriptorData)
    {
        create_defaults();
    }

    Handle<AssetNode> load_gltf_asset(const char* filepath);

    void set_skybox(const char* filepath);

    void add_asset_to_entity(Entity entity,
                          Handle<AssetNode> assetHandle,
                          Resource<World>& world) const;

  private:
    [[nodiscard]] Material get_material(Handle<Material> material) const;
    [[nodiscard]] Mesh get_mesh(Handle<Mesh> mesh) const;

    void create_defaults();

    std::optional<AllocatedImage> load_image(fastgltf::Asset& asset,
                                            fastgltf::Image& gltfImage);

    AllocatedImage load_skybox_image(const char* filepath);

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

    std::unordered_map<const char*, Handle<AssetNode>> pathToAssetMap;

    // Assets, buffers, descriptors are coupled
    // These might be combined into a single struct later?
    std::vector<AssetNode> assets;
    std::vector<DescriptorAllocator> allocators;

    // Colour, RoughnessMetallic, Normal textures
    std::vector<AllocatedImage> images;
    // Image samplers
    std::vector<VkSampler> samplers;

    // The CPU side data is purely for indexing into the mega buffers
    std::vector<Mesh> meshes;
    std::vector<Material> materials;

    MegaBuffer verticeBuffer;
    MegaBuffer indiceBuffer;
    MegaBuffer materialBuffer;

    size_t skyboxTextureIndex;
    AllocatedMeshBuffer skyboxCube;

    DescriptorWriter descriptorWriter;
    TextureCache textureCache;

    VulkanContext& context;
    VmaAllocator& allocator;
    ImmediateSubmit& immediate;
    GraphicsQueue& graphicsQueue;
    GlobalDescriptorData& globalDescriptorData;

    friend class DrawData;
    friend void cleanup_asset_server(Resource<AssetServer>& assetServer);
};
}
