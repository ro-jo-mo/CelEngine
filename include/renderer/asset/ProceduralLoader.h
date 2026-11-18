#pragma once

#include "AssetTypes.h"
#include "common/Handle.h"
#include "renderer/passes/StagingPool.h"

#include <glm/vec3.hpp>
#include <mutex>
#include <variant>
#include <vector>

namespace Cel::Renderer::Asset {

class ProceduralLoader
{
    struct PrimitiveData;

  public:
    Handle<Primitive> create_cube(glm::vec3 size = glm::vec3(1.0f));

    // Create an image from this pixel data. Makes several assumptions about the
    // usage
    Handle<ImageAsset> create_image(const std::vector<uint32_t>& pixels,
                                    VkFormat format,
                                    VkExtent3D extent);

    Primitive& get_primitive(Handle<Primitive> handle);

    ImageAsset& get_image(Handle<ImageAsset> handle);

    void flush_cpu(VulkanResourceManager& manager,
                   SharedAssetData& shared,
                   RenderGraph::PassBuilder& pass);

    void flush_gpu(VkCommandBuffer cmd, const RenderGraph::PassServer& server);

  private:
    struct Cube
    {
        glm::vec3 size;
    };
    struct Sphere
    {
        float radius;
    };

    using Shape = std::variant<Cube, Sphere>;

    struct PrimitiveData
    {
        Handle<Primitive> handle;
        Shape shape;

        // Filled in by prepare
        CpuPrimitive primitive;
    };

    struct TextureData
    {
        Handle<ImageAsset> handle;

        VkFormat format;
        VkExtent3D extent;

        std::vector<uint32_t> pixels;
    };

    /**
     * Generates the geometry for the requested shape
     * The data created here is kept exclusively here until commit
     * Can be launched asynchronously
     * @param data
     */
    void prepare(PrimitiveData data);

    /**
     * Allocates the primitive in the shared buffers
     * Single threaded access
     */
    void commit(const PrimitiveData& data, SharedAssetData& shared);

    void commit(const TextureData& data,
                VulkanResourceManager& manager,
                SharedAssetData& shared,
                RenderGraph::PassBuilder& pass);

    static void generate_cube(const Cube& cube, CpuPrimitive& primitive);

    std::mutex processingLock;
    std::vector<PrimitiveData> primitivesToProcess;
    std::vector<TextureData> texturesToProcess;

    std::vector<Primitive> primitives;

    std::vector<ImageAsset> images;

    StagingPool stagingPool;
};

}
