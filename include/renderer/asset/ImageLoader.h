#pragma once

#include "AssetTypes.h"
#include "renderer/RenderTypes.h"
#include "renderer/passes/StagingPool.h"
#include "renderer/render-graph/PassServer.h"

#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

struct ktxTexture;

namespace Cel::Renderer {
class VulkanResourceManager;
}

namespace Cel::Renderer::RenderGraph {
class PassBuilder;
}
namespace Cel::Renderer::Asset {

/**
 * The only async command in this class is "prepare"
 */
class ImageLoader
{
    struct ImageData;

  public:
    explicit ImageLoader(VkDevice device);

    Handle<TextureAsset> load_texture(const std::string& file,
                                      SamplerSettings sampler);

    Handle<ImageAsset> load_image(const std::string& file);

    ImageAsset& get_image(Handle<ImageAsset> handle);

    uint32_t get_texture_index(Handle<TextureAsset> handle);

    void flush_cpu(VulkanResourceManager& manager,
                   SharedAssetData& shared,
                   RenderGraph::PassBuilder& pass);

    void flush_gpu(VkCommandBuffer cmd, const RenderGraph::PassServer& server);

    void declare_image_access(RenderGraph::PassBuilder& pass) const;

    void cleanup();

  private:
    struct ImageData
    {
        Handle<ImageAsset> handle;
        std::filesystem::path file;

        // Freed in flush_gpu
        ktxTexture* texture;
        ImageRequirements requirements;
        size_t size;
    };

    struct TextureData
    {
        Handle<TextureAsset> texHandle;
        Handle<ImageAsset> imgHandle;
        SamplerSettings sampler;
    };

    /**
     * Does the initial loading of the image, preparing the data for use
     * The data created here is kept exclusively here until commit
     * Can be launched asynchronously
     * @param handle
     * @param file
     */
    void prepare(ImageData data);

    /**
     * Creates the gpu image and registers the upload
     * Single threaded access
     */
    void commit(const ImageData& data,
                VulkanResourceManager& manager,
                RenderGraph::PassBuilder& pass);

    static void load_ktx(ImageData& data);

    std::mutex processingLock;
    std::vector<ImageData> imagesToProcess;
    std::vector<TextureData> texturesToProcess;

    std::vector<ImageAsset> images;

    // A simple mapping of texture handle to index in the texture cache
    std::vector<uint32_t> textureIndices;

    // Owned here, destroyed in cleanup
    std::vector<VkSampler> samplers;

    StagingPool stagingPool;

    VkDevice device;
};

}
