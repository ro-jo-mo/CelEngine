#pragma once

#include "../core/World.h"
#include "Descriptors.h"
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
                Resource<GraphicsQueue>& graphicsQueue)
        : context(*context)
        , allocator(*allocator)
        , immediate(*immediate)
        , graphicsQueue(*graphicsQueue)
    {
        CreateDefaults();
    }

    Handle<AssetNode> LoadAsset(const char* filepath);
    void AddAssetToEntity(Entity entity,
                          Handle<AssetNode> assetHandle,
                          Resource<World>& world) const;
    VkDescriptorSetLayout materialLayout;

  private:
    void CreateDefaults();

    std::optional<AllocatedImage> LoadImage(fastgltf::Asset& asset,
                                            fastgltf::Image& gltfImage);

    void LoadImages(fastgltf::Asset& asset);
    void LoadSamplers(const fastgltf::Asset& asset);
    void WriteMaterialDescriptors(Material& material,
                                  DescriptorAllocator& descriptorAllocator);
    uint32_t ResolveTextureSampler(
        fastgltf::Asset& asset,
        const std::optional<fastgltf::TextureInfo>& textureInfo,
        size_t imageOffset,
        size_t samplerOffset);

    void LoadMaterials(fastgltf::Asset& asset,
                       DescriptorAllocator& descriptorAllocator,
                       size_t imageOffset,
                       size_t samplerOffset);
    AssetNode LoadNodes(fastgltf::Asset& asset, std::vector<Model>& models);
    std::vector<Model> LoadModels(fastgltf::Asset& asset,
                                  size_t materialOffset);

    std::unordered_map<const char*, Handle<AssetNode>> pathToAssetMap;

    // Assets, buffers, descriptors are coupled
    // These might be combined into a single struct later?
    std::vector<AssetNode> assets;
    std::vector<DescriptorAllocator> allocators;
    std::vector<AllocatedBuffer> materialBuffers;

    std::vector<AllocatedImage> images;
    std::vector<VkSampler> samplers;
    std::vector<Mesh> meshes;
    std::vector<AllocatedMeshBuffer> meshBuffers;
    std::vector<Material> materials;

    DescriptorWriter descriptorWriter;
    TextureCache textureCache;

    VulkanContext& context;
    VmaAllocator& allocator;
    ImmediateSubmit& immediate;
    GraphicsQueue& graphicsQueue;
};
}
