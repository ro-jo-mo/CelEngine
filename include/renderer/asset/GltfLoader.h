#pragma once

#include "AssetTypes.h"
#include "GltfLabels.h"
#include "renderer/resource-management/MegaBuffer.h"

#include <string>

namespace Cel::Renderer::Asset {

// Gltf -> Scene
// Scene stores models
//

// A mesh is made up of many primitives.
// Each primitive has its own material

class GltfLoader
{
    struct GltfData;

  public:
    /**
     * Does the initial loading of the asset, preparing the data for use
     * The data created here is kept exclusively here until commit
     * Can be launched asynchronously
     * @param file
     * @param label
     */
    void prepare(std::string file, GltfLabel::Scene label);

    /**
     *
     */
    void flush_cpu(VulkanResourceManager& manager, SharedAssetData& shared);

  private:
    struct GltfData
    {
        std::vector<Mesh> meshes;
        std::vector<CpuPrimitive> primitives;
        // image,sampler indices
        std::vector<std::pair<uint32_t, uint32_t>> textureSamplers;
        // Texture indices are unset until commit
        // Based on the result of adding textureSamplers to the texture cache
        std::vector<MaterialConstants> materials;

        std::vector<VkSampler> samplers;
        std::vector<std::pair<void*, VkExtent3D>> images;
        SceneAsset scene;

        uint32_t imageOffset;
        uint32_t samplerOffset;
    };

    /**
     * Finalises the changes, adding them to shared storage
     * Single threaded access
     */
    void commit(GltfData& data,
                VulkanResourceManager& manager,
                SharedAssetData& shared);

    static void load_meshes(fastgltf::Asset& asset, GltfData& data);

    void load_samplers(const fastgltf::Asset& asset, GltfData& data) const;

    static void load_materials(fastgltf::Asset& asset, GltfData& data);

    static void load_images(fastgltf::Asset& asset, GltfData& data);

    static void load_nodes(fastgltf::Asset& asset, GltfData& data);

    static SceneAsset load_scene_tree(size_t nodeIndex,
                                      fastgltf::Asset& asset,
                                      GltfData& data);

    std::mutex processingLock;
    std::vector<GltfData> processing;

    std::vector<AllocatedImage> images;
    std::vector<VkSampler> samplers;

    VkDevice device;
};

}