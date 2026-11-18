#include "renderer/asset/ProceduralLoader.h"

#include "common/Misc.h"
#include "core/Error.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <initializer_list>

Cel::Handle<Cel::Renderer::Asset::Primitive>
Cel::Renderer::Asset::ProceduralLoader::create_cube(const glm::vec3 size)
{
    const Handle<Primitive> handle{ static_cast<uint32_t>(primitives.size()) };

    primitives.emplace_back();

    prepare({ .handle = handle, .shape = Cube{ .size = size } });

    return handle;
}

Cel::Handle<Cel::Renderer::Asset::ImageAsset>
Cel::Renderer::Asset::ProceduralLoader::create_image(
    const std::vector<uint32_t>& pixels,
    const VkFormat format,
    VkExtent3D extent)
{
    const Handle<ImageAsset> handle{ static_cast<uint32_t>(images.size()) };

    images.emplace_back();

    texturesToProcess.push_back({ .handle = handle,
                                  .format = format,
                                  .extent = extent,
                                  .pixels = pixels });

    return handle;
}

Cel::Renderer::Asset::Primitive&
Cel::Renderer::Asset::ProceduralLoader::get_primitive(
    const Handle<Primitive> handle)
{
    return primitives[handle.index];
}

Cel::Renderer::Asset::ImageAsset&
Cel::Renderer::Asset::ProceduralLoader::get_image(
    const Handle<ImageAsset> handle)
{
    return images[handle.index];
}

void
Cel::Renderer::Asset::ProceduralLoader::prepare(PrimitiveData data)
{
    std::visit(
        Overload{
            [&](const Cube& cube) { generate_cube(cube, data.primitive); },
            [](const Sphere& sphere) { throw_error(("unimplemented")); } },
        data.shape);

    std::unique_lock guard{ processingLock };
    primitivesToProcess.push_back(std::move(data));
}

void
Cel::Renderer::Asset::ProceduralLoader::generate_cube(const Cube& cube,
                                                      CpuPrimitive& primitive)
{
    // diagonal diagonal normal
    const glm::vec3 corners[] = {
        // Top face
        { -1, 1, -1 },
        { 1, 1, 1 },
        { 1, 1, -1 },
        { -1, 1, 1 },

        { 0, 1, 0 },
        // Left face
        { -1, 1, -1 },
        { -1, -1, 1 },
        { -1, 1, 1 },
        { -1, -1, -1 },

        { -1, 0, 0 },
        // Right face
        { 1, 1, 1 },
        { 1, -1, -1 },
        { 1, 1, -1 },
        { 1, -1, 1 },

        { 1, 0, 0 },
        // Bottom face
        { -1, -1, 1 },
        { 1, -1, -1 },
        { 1, -1, 1 },
        { -1, -1, -1 },

        { 0, -1, 0 },
        // Rear face
        { 1, 1, -1 },
        { -1, -1, -1 },
        { -1, 1, -1 },
        { 1, -1, -1 },

        { 0, 0, -1 },
        // Front face
        { -1, 1, 1 },
        { 1, -1, 1 },
        { 1, 1, 1 },
        { -1, -1, 1 },

        { 0, 0, 1 },
    };

    const glm::vec2 uvs[] = { { 0, 0 }, { 1, 1 }, { 1, 0 }, { 0, 1 } };

    // counterclockwise winding
    const uint32_t order[] = { 0, 3, 2, 1, 2, 3 };

    primitive.vertices.reserve(24);
    primitive.indices.reserve(36);

    for (uint32_t i = 0; i < 30; i += 5) {
        const auto base = static_cast<uint32_t>(primitive.vertices.size());

        for (uint32_t corner = 0; corner < 4; ++corner) {
            primitive.vertices.push_back(
                { .position = corners[i + corner] * cube.size,
                  .uvX = uvs[corner].x,
                  .normal = corners[i + 4],
                  .uvY = uvs[corner].y });
        }

        for (const uint32_t corner : order) {
            primitive.indices.push_back(base + corner);
        }
    }
}

void
Cel::Renderer::Asset::ProceduralLoader::flush_cpu(
    VulkanResourceManager& manager,
    SharedAssetData& shared,
    RenderGraph::PassBuilder& pass)
{
    processingLock.lock();

    stagingPool.reset();

    for (const auto& data : primitivesToProcess) {
        commit(data, shared);
    }

    for (const auto& data : texturesToProcess) {
        commit(data, manager, shared, pass);
    }
}

void
Cel::Renderer::Asset::ProceduralLoader::flush_gpu(
    VkCommandBuffer cmd,
    const RenderGraph::PassServer& server)
{
    // Resetting simply ticks the handles back to the start of the list.
    // As we iterate in the same order, this gives us the same handle for each
    // image
    stagingPool.reset();

    for (const auto& data : texturesToProcess) {
        auto staging = server.get_resource(stagingPool.get_handle());

        Utils::upload_image_asset(
            data.pixels.data(), cmd, images[data.handle.index].image, staging);
    }

    // Finally we can clear the processed data and unlock
    primitivesToProcess.clear();
    texturesToProcess.clear();

    processingLock.unlock();
}

void
Cel::Renderer::Asset::ProceduralLoader::commit(const PrimitiveData& data,
                                               SharedAssetData& shared)
{
    const auto& [vertices, indices] = data.primitive;
    auto& primitive = primitives[data.handle.index];

    primitive.firstIndex =
        shared.indexBuffer.allocate(indices.data(),
                                    indices.size() * sizeof(uint32_t)) /
        sizeof(uint32_t);

    primitive.indexCount = indices.size();

    primitive.vertexOffset =
        shared.vertexBuffer.allocate(vertices.data(),
                                     vertices.size() * sizeof(Vertex)) /
        sizeof(Vertex);
}

void
Cel::Renderer::Asset::ProceduralLoader::commit(const TextureData& data,
                                               VulkanResourceManager& manager,
                                               SharedAssetData& shared,
                                               RenderGraph::PassBuilder& pass)
{
    const ImageRequirements req{ .format = data.format,
                                 .extent = data.extent,
                                 .usages = VK_IMAGE_USAGE_SAMPLED_BIT |
                                           VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                 .aspects = VK_IMAGE_ASPECT_COLOR_BIT };

    const auto handle =
        manager.get_handle_from_requirements(req, "procedural_image");

    ImageAsset img = { .image = manager.get_resource_from_handle(handle),
                       .mipLevels = req.mipLevels,
                       .arrayLayers = req.arrayLayers,
                       .viewType = req.viewType };

    img.texture = shared.textures.add_texture(img.image.imageView);

    images[data.handle.index] = img;

    pass.upload_image(data.pixels.size() * sizeof(uint32_t),
                      stagingPool.get_handle(),
                      handle);
}
