#include "renderer/asset/ImageLoader.h"

#include "core/Error.h"
#include "renderer/VulkanUtils.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <filesystem>
#include <ktx.h>

Cel::Renderer::Asset::ImageLoader::ImageLoader(VkDevice device)
    : device(device)
{
}

Cel::Handle<Cel::Renderer::Asset::TextureAsset>
Cel::Renderer::Asset::ImageLoader::load_texture(const std::string& file,
                                                SamplerSettings sampler)
{
    const Handle<TextureAsset> texHandle{ static_cast<uint32_t>(
        textureIndices.size()) };

    textureIndices.emplace_back();

    const Handle<ImageAsset> imgHandle{ static_cast<uint32_t>(images.size()) };

    images.emplace_back();

    texturesToProcess.emplace_back(texHandle, imgHandle, sampler);

    // Move to passing the data struct in from the load_ method?
    prepare({ .handle = imgHandle, .file = file });

    return texHandle;
}

Cel::Handle<Cel::Renderer::Asset::ImageAsset>
Cel::Renderer::Asset::ImageLoader::load_image(const std::string& file)
{
    const Handle<ImageAsset> handle{ static_cast<uint32_t>(images.size()) };

    images.emplace_back();

    prepare({ .handle = handle, .file = file });

    return handle;
}

Cel::Renderer::Asset::ImageAsset&
Cel::Renderer::Asset::ImageLoader::get_image(Handle<ImageAsset> handle)
{
    return images[handle.index];
}

uint32_t
Cel::Renderer::Asset::ImageLoader::get_texture_index(
    Handle<TextureAsset> handle)
{
    return textureIndices[handle.index];
}

void
Cel::Renderer::Asset::ImageLoader::prepare(ImageData data)
{
    const auto extension = data.file.extension();

    if (extension == ".ktx2") {
        load_ktx(data);
    } else {
        throw_error("Unsupported image format {} ({})",
                    extension.string(),
                    std::filesystem::absolute(data.file).string());
    }

    std::unique_lock guard{ processingLock };
    imagesToProcess.push_back(std::move(data));
}

void
Cel::Renderer::Asset::ImageLoader::load_ktx(ImageData& data)
{
    auto ktx_error = [&](auto& error) {
        if (error != KTX_SUCCESS) {
            throw_error("Failed to load image {} ktx error: {}",
                        std::filesystem::absolute(data.file).string(),
                        ktxErrorString(error));
        }
    };

    // ktx2 stores the vk format
    // I could probably guess for a ktx1
    ktxTexture2* texture2;

    const auto loadError = ktxTexture2_CreateFromNamedFile(
        data.file.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture2);

    ktx_error(loadError);

    auto texture = ktxTexture(texture2);

    if (ktxTexture_NeedsTranscoding(texture)) {
        const auto transcodeError =
            ktxTexture2_TranscodeBasis(texture2, KTX_TTF_RGBA32, 0);

        ktx_error(transcodeError);
    }

    // At some point I will add LUT / 3D img support. For now error!
    if (texture->numDimensions != 2 || texture->numLayers != 1) {
        ktxTexture_Destroy(texture);
        throw_error("Unsupported ktx image {}. Only 2D images and cubemaps "
                    "are supported",
                    data.file.c_str());
    }

    VkImageCreateFlags flags = 0;
    VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
    if (texture->numFaces == 6) {
        flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }

    data.texture = texture;
    data.size = ktxTexture_GetDataSize(texture);
    data.requirements = { .format = static_cast<VkFormat>(texture2->vkFormat),
                          .extent = { .width = texture->baseWidth,
                                      .height = texture->baseHeight,
                                      .depth = 1 },
                          .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                    VK_IMAGE_USAGE_SAMPLED_BIT,
                          .aspects = VK_IMAGE_ASPECT_COLOR_BIT,
                          .mipLevels = texture->numLevels,
                          .arrayLayers = texture->numFaces,
                          .flags = flags,
                          .viewType = viewType };

    if (data.requirements.format == VK_FORMAT_UNDEFINED) {
        ktxTexture_Destroy(texture);
        throw_error("Ktx file has no vk format {}", data.file.c_str());
    }
}

void
Cel::Renderer::Asset::ImageLoader::flush_cpu(VulkanResourceManager& manager,
                                             SharedAssetData& shared,
                                             RenderGraph::PassBuilder& pass)
{
    // We maintain a lock until we've completely finished with this batch
    processingLock.lock();
    stagingPool.reset();

    for (const auto& data : imagesToProcess) {
        commit(data, manager, pass);
    }

    // I'll have to check the image is loaded for async
    for (const auto& data : texturesToProcess) {

        VkSampler sampler;

        VkSamplerCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .magFilter = data.sampler.mag,
            .minFilter = data.sampler.min,
            .mipmapMode = data.sampler.mipmapMode,
            .addressModeU = data.sampler.addressMode,
            .addressModeV = data.sampler.addressMode,
            .addressModeW = data.sampler.addressMode,
            .minLod = 0,
            .maxLod = VK_LOD_CLAMP_NONE,
        };

        vkCreateSampler(device, &info, nullptr, &sampler);
        samplers.push_back(sampler);

        textureIndices[data.texHandle.index] = shared.textures.add_texture(
            images[data.imgHandle.index].image.imageView, sampler);
    }

    texturesToProcess.clear();
}

void
Cel::Renderer::Asset::ImageLoader::commit(const ImageData& data,
                                          VulkanResourceManager& manager,
                                          RenderGraph::PassBuilder& pass)
{
    const auto handle = manager.get_handle_from_requirements(
        data.requirements, data.file.filename());

    images[data.handle.index] = { .image =
                                      manager.get_resource_from_handle(handle),
                                  .mipLevels = data.requirements.mipLevels,
                                  .arrayLayers = data.requirements.arrayLayers,
                                  .viewType = data.requirements.viewType };

    pass.upload_image(data.size, stagingPool.get_handle(), handle);
}

void
Cel::Renderer::Asset::ImageLoader::flush_gpu(
    VkCommandBuffer cmd,
    const RenderGraph::PassServer& server)
{
    // Resetting simply ticks the handles back to the start of the list.
    // As we iterate in the same order, this gives us the same handle for each
    // image
    stagingPool.reset();

    for (const auto& data : imagesToProcess) {
        auto staging = server.get_resource(stagingPool.get_handle());

        Utils::upload_image_asset(
            data.texture, cmd, images[data.handle.index].image, staging);

        ktxTexture_Destroy(data.texture);
    }

    // Finally we can clear the processed data and unlock
    imagesToProcess.clear();
    processingLock.unlock();
}

void
Cel::Renderer::Asset::ImageLoader::declare_image_access(
    RenderGraph::PassBuilder& pass) const
{
    for (const auto& img : images) {

        pass.read_image(img.image.handle,
                        VK_ACCESS_2_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

void
Cel::Renderer::Asset::ImageLoader::cleanup()
{
    // Anything that was never flushed still owns its ktx data
    for (const auto& data : imagesToProcess) {
        ktxTexture_Destroy(data.texture);
    }
    imagesToProcess.clear();

    for (const auto sampler : samplers) {
        vkDestroySampler(device, sampler, nullptr);
    }
    samplers.clear();
}
