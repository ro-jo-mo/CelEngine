#pragma once

#include "AssetTypes.h"
#include "GltfLabels.h"
#include "renderer/resource-management/MegaBuffer.h"

#include <optional>
#include <string>

namespace Cel::Renderer::Asset {

// Gltf -> Scene
// Scene stores models
//

// A mesh is made up of many primitives.
// Each primitive has its own material

class GltfLoader
{
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
     * Finalises the changes, adding them to shared storage
     * Single threaded access
     */
    void commit();

  private:
    struct GltfData
    {
        std::vector<Mesh> meshes;
        std::vector<CpuPrimitive> primitives;
        std::vector<MaterialConstants> materials;
        std::vector<VkSampler> samplers;
        std::vector<std::pair<void*, VkExtent3D>> images;
    };

    static void load_meshes(fastgltf::Asset& asset, GltfData& data);

    void load_samplers(const fastgltf::Asset& asset, GltfData& data) const;

    static void load_materials(fastgltf::Asset& asset, GltfData& data);

    static void load_images(fastgltf::Asset& asset, GltfData& data);

    std::vector<GltfData> processing;

    SharedAssetData& shared;
};

}