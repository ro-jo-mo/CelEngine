#pragma once

#include "AssetTypes.h"
#include "GltfLabels.h"
#include "renderer/passes/StagingPool.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/resource-management/MegaBuffer.h"

#include <fastgltf/types.hpp>
#include <string>

namespace Cel::Renderer::RenderGraph {
class PassBuilder;
}
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
    auto load_gltf(std::string file);

    void flush_cpu(VulkanResourceManager& manager,
                   SharedAssetData& shared,
                   RenderGraph::PassBuilder& pass);

    void flush_gpu(VkCommandBuffer cmd, const RenderGraph::PassServer& server);

  private:
    struct GltfData
    {
        Handle<SceneAsset> handle;

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
    };

    /**
     * Does the initial loading of the asset, preparing the data for use
     * The data created here is kept exclusively here until commit
     * Can be launched asynchronously
     * @param file
     * @param label
     */
    void prepare(Handle<SceneAsset> handle,
                 std::string file,
                 GltfLabel::Scene label);

    /**
     * Finalises the changes, adding them to shared storage
     * Single threaded access
     */
    void commit(GltfData& data,
                uint32_t samplerOffset,
                uint32_t primitiveOffset,
                uint32_t materialOffset,

                VulkanResourceManager& manager,
                SharedAssetData& shared,
                RenderGraph::PassBuilder& pass);

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

    std::vector<SceneAsset> scenes;

    std::vector<AllocatedImage> images;
    std::vector<VkSampler> samplers;
    std::vector<Primitive> primitives;
    std::vector<Material> materials;

    StagingPool stagingPool;

    VkDevice device;
};

}