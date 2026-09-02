#define STB_IMAGE_IMPLEMENTATION

#include "renderer/AssetServer.h"

#include "core/Error.h"
#include "renderer/VulkanUtils.h"
#include "renderer/passes/HandleAllocator.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/glm.hpp>
#include <ktx.h>
#include <ranges>
#include <stb_image.h>

using namespace Cel::Renderer::Assets;
using namespace Cel::Renderer;
using namespace Cel;

// TEMP NOTES
// We can use the gltf load mesh to just get the cubemap mesh
// Uploads to the megabuffer like every other mesh
//

AssetServer::AssetServer(VulkanResourceManager& manager)
    : vertexBuffer({ .allocSize = 2 << 16,
                     .usages = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY },
                   "vertice_mega_buffer_alloc",
                   manager)
    , indiceBuffer({ .allocSize = 2 << 16,
                     .usages = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                               VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY },
                   "indice_mega_buffer_alloc",
                   manager)
    , materialBuffer({ .allocSize = 2 << 16,
                       .usages = VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                       .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY },
                     "material_mega_buffer_alloc",
                     manager)

{
    create_defaults(manager);
}

void
AssetServer::create_defaults(VulkanResourceManager& manager)
{
    // Reserve handles
    reservedBufferHandles.reserve(512);
    for (uint32_t i = 0; i < 512; i++) {
        reservedBufferHandles.emplace_back(
            Passes::HandleAllocator::allocate_buffer("asset_server_reserved"));
    }

    // checkerboard image
    int32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    int32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));

    // for 16x16 checkerboard texture
    auto pixels = static_cast<uint32_t*>(malloc(16 * 16 * sizeof(uint32_t)));
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            pixels[y * 16 + x] = (x % 2) ^ (y % 2) ? magenta : black;
        }
    }

    // For now, we'll default to a checkerboard when no texture is assigned
    // However, this is a little silly for normals roughness etc
    cmdCreateImgs.push_back({ pixels, { 16, 16, 1 }, false, true });

    VkSampler defaultSampler;
    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
    };

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    vkCreateSampler(device, &samplerInfo, nullptr, &defaultSampler);
    samplers.push_back(defaultSampler);
}

std::vector<Model>
AssetServer::load_models(fastgltf::Asset& asset, size_t materialOffset)
{
    std::vector<Model> models;

    for (const auto& gltfMesh : asset.meshes) {
        Model newModel;

        for (const auto& gltfPrimitive : gltfMesh.primitives) {
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            // Associate the new mesh with a material
            fastgltf::Optional<uint32_t> materialIndex{};
            if (materialIndex.has_value()) {
                materialIndex =
                    materialIndex.value() * sizeof(MaterialConstants) +
                    materialOffset;
            }

            newModel.materials.push_back(materialIndex);

            // Load indices
            {
                fastgltf::Accessor& accessor =
                    asset.accessors[gltfPrimitive.indicesAccessor.value()];

                indices.reserve(accessor.count);

                fastgltf::iterateAccessor<uint32_t>(
                    asset, accessor, [&](const uint32_t idx) {
                        indices.push_back(idx);
                    });
            }

            // Load vertices
            {
                fastgltf::Accessor& accessor =
                    asset.accessors[gltfPrimitive.findAttribute("POSITION")
                                        ->accessorIndex];

                vertices.reserve(accessor.count);

                fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                    asset, accessor, [&](fastgltf::math::fvec3 vert) {
                        Vertex newVertex;
                        newVertex.position = { vert.x(), vert.y(), vert.z() };
                        newVertex.normal = { 1, 0, 0 };
                        newVertex.uv_x = 0;
                        newVertex.uv_y = 0;
                        vertices.push_back(newVertex);
                    });
            }

            // Load normals
            {
                const auto normals = gltfPrimitive.findAttribute("NORMALS");

                if (normals != gltfPrimitive.attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                        asset,
                        asset.accessors[normals->accessorIndex],
                        [&](fastgltf::math::fvec3 normal, const size_t index) {
                            vertices[index].normal = { normal.x(),
                                                       normal.y(),
                                                       normal.z() };
                        });
                }
            }

            // Load uvs
            {
                const auto uv = gltfPrimitive.findAttribute("TEXCOORD_0");

                if (uv != gltfPrimitive.attributes.end()) {
                    fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                        asset,
                        asset.accessors[uv->accessorIndex],
                        [&](fastgltf::math::fvec2 vert, const size_t index) {
                            vertices[index].uv_x = vert.x();
                            vertices[index].uv_y = vert.y();
                        });
                }

                newModel.meshes.push_back(meshes.size());

                Mesh newMesh{};
                newMesh.firstIndex = indiceBuffer.allocate(
                    indices.data(), indices.size() * sizeof(uint32_t));
                newMesh.indexCount = indices.size();
                newMesh.vertexOffset = vertexBuffer.allocate(
                    vertices.data(), vertices.size() * sizeof(Vertex));

                meshes.push_back(newMesh);
            }
        }
        models.push_back(newModel);
    }

    return models;
}

void
AssetServer::load_image(fastgltf::Asset& asset, fastgltf::Image& gltfImage)
{

    std::vector<std::byte> imageData;

    auto ErrorMsg = [&](const char* ext) {
        (throw_error("Error loading gltf image. Attempted to use {}",
                     std::move(ext)));
    };

    std::visit(
        fastgltf::visitor{
            [&](const fastgltf::sources::Array& array) {
                imageData.assign(array.bytes.begin(), array.bytes.end());
            },
            [&](const fastgltf::sources::BufferView& bufferView) {
                auto& view = asset.bufferViews[bufferView.bufferViewIndex];
                auto& buffer = asset.buffers[view.bufferIndex];
                auto& data = std::get<fastgltf::sources::Array>(buffer.data);
                auto begin = data.bytes.begin() + view.byteOffset;
                imageData.assign(begin, begin + view.byteLength);
            },
            [&](const fastgltf::sources::URI&) { ErrorMsg("URI"); },
            [&](const fastgltf::sources::Vector&) { ErrorMsg("Vector"); },
            [&](const std::monostate&) { ErrorMsg("Monostate"); },
            [&](const fastgltf::sources::CustomBuffer&) {
                ErrorMsg("CustomBuffer");
            },
            [&](const fastgltf::sources::ByteView&) { ErrorMsg("ByteView"); },
            [&](const fastgltf::sources::Fallback&) { ErrorMsg("Fallback"); } },
        gltfImage.data);

    if (imageData.empty()) {

        throw_error("Failed to load an image ({}) in gltf asset",
                    std::move(gltfImage.name));
    }

    int width, height, channels;

    auto img = stbi_load_from_memory(
        reinterpret_cast<stbi_uc const*>(imageData.data()),
        imageData.size(),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha);

    VkExtent3D size;
    size.width = width;
    size.height = height;
    size.depth = 1;

    cmdCreateImgs.emplace_back(img, size, false, true);
}

void
AssetServer::load_images(fastgltf::Asset& asset)
{
    for (auto& image : asset.images) {
        load_image(asset, image);
    }
}

VkFilter
ExtractFilter(const fastgltf::Filter filter)
{
    switch (filter) {
        // nearest samplers
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::NearestMipMapLinear:
            return VK_FILTER_NEAREST;

            // linear samplers
        case fastgltf::Filter::Linear:
        case fastgltf::Filter::LinearMipMapNearest:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_FILTER_LINEAR;
    }
}

VkSamplerMipmapMode
ExtractMipMap(const fastgltf::Filter filter)
{
    switch (filter) {
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::LinearMipMapNearest:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

void
AssetServer::load_samplers(const fastgltf::Asset& asset)
{
    for (auto& gltfSampler : asset.samplers) {
        VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr
        };
        sampler.maxLod = VK_LOD_CLAMP_NONE;
        sampler.minLod = 0;

        sampler.magFilter = ExtractFilter(
            gltfSampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampler.minFilter = ExtractFilter(
            gltfSampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampler.mipmapMode = ExtractMipMap(
            gltfSampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler;
        vkCreateSampler(device, &sampler, nullptr, &newSampler);

        samplers.push_back(newSampler);
    }
}

uint32_t
AssetServer::resolve_texture_sampler(
    fastgltf::Asset& asset,
    const std::optional<fastgltf::TextureInfo>& textureInfo,
    const size_t imageOffset,
    const size_t samplerOffset)
{

    if (textureInfo.has_value()) {
        auto& texture = asset.textures[textureInfo.value().textureIndex];
        const auto view =
            gltfImages[texture.imageIndex.value() + imageOffset].imageView;

        VkSampler sampler;
        if (texture.samplerIndex.has_value()) {
            sampler = samplers[texture.samplerIndex.value() + samplerOffset];
        } else {
            sampler = samplers[0];
        }

        return textureCache.add_texture(view, sampler);
    }
    return textureCache.add_texture(gltfImages[0].imageView, samplers[0]);
}

void
AssetServer::load_materials(fastgltf::Asset& asset,
                            const size_t imageOffset,
                            const size_t samplerOffset)
{
    std::vector<MaterialConstants> materialList;

    for (auto& gltfMaterial : asset.materials) {
        // Material constants will be stored in a buffer on the gpu
        // The material will then contain the buffer index and offset for these
        // constants
        MaterialConstants constants{};

        const auto& [baseColorFactor,
                     metallicFactor,
                     roughnessFactor,
                     baseColorTexture,
                     metallicRoughnessTexture] = gltfMaterial.pbrData;

        auto& colour = baseColorFactor;
        constants.baseColorFactors = {
            colour.x(), colour.y(), colour.z(), colour.w()
        };
        constants.metalRoughnessFactors = {
            metallicFactor, roughnessFactor, 0, 0
        };

        constants.colorTextureIndex = resolve_texture_sampler(
            asset, baseColorTexture, imageOffset, samplerOffset);
        constants.metalRoughnessTextureIndex = resolve_texture_sampler(
            asset, metallicRoughnessTexture, imageOffset, samplerOffset);
        constants.normalTextureIndex = resolve_texture_sampler(
            asset,
            gltfMaterial.normalTexture.transform([](const auto& info) {
                return fastgltf::TextureInfo{ .textureIndex = info.textureIndex,
                                              .texCoordIndex =
                                                  info.texCoordIndex };
            }),
            imageOffset,
            samplerOffset);

        materialList.push_back(constants);
    }

    const auto bufferOffset = materialBuffer.allocate(
        materialList.data(), sizeof(MaterialConstants) * materialList.size());

    for (uint32_t i = 0; i < materialList.size(); i++) {
        materials.push_back(Material{ .bufferIndex = bufferOffset + i });
    }
}

AssetNode
CreateNodeTree(const size_t nodeIndex,
               fastgltf::Asset& asset,
               std::vector<Model>& models)
{
    auto node = asset.nodes[nodeIndex];

    AssetNode newNode{};
    newNode.children.reserve(node.children.size());
    newNode.name = node.name;
    if (node.meshIndex.has_value()) {
        newNode.model = models[node.meshIndex.value()];
    }

    std::visit(
        fastgltf::visitor{
            [&](fastgltf::TRS& trs) {
                glm::vec3 t = { trs.translation.x(),
                                trs.translation.y(),
                                trs.translation.z() };
                glm::quat r = {
                    trs.rotation.w(),
                    trs.rotation.x(),
                    trs.rotation.y(),
                    trs.rotation.z(),
                };
                glm::vec3 s = { trs.scale.x(), trs.scale.y(), trs.scale.z() };

                newNode.localTransform = glm::translate(glm::mat4(1.0f), t) *
                                         glm::mat4_cast(glm::quat(r)) *
                                         glm::scale(glm::mat4(1.0f), s);
            },
            [&](fastgltf::math::fmat4x4& mat) {
                memcpy(&newNode.localTransform, mat.data(), sizeof(mat));
            } },
        node.transform);

    for (auto const child : node.children) {
        newNode.children.push_back(CreateNodeTree(child, asset, models));
    }

    return newNode;
}

AssetNode
AssetServer::load_nodes(fastgltf::Asset& asset, std::vector<Model>& models)
{
    // A scene may have several root nodes
    // Return a list of root nodes? The roots contain their children
    auto& scene = asset.scenes[asset.defaultScene.value_or(0)];

    std::vector<AssetNode> roots;
    roots.reserve(scene.nodeIndices.size());

    for (const auto& index : scene.nodeIndices) {
        roots.push_back(CreateNodeTree(index, asset, models));
    }

    std::vector<size_t> rootIds(roots.size());

    AssetNode root = { .name = "",
                       .children = roots,
                       .localTransform = glm::mat4(1.0f) };

    return root;
}

Handle<AssetNode>
AssetServer::load_gltf_asset(const char* filepath)
{
    std::filesystem::path path = filepath;

    fastgltf::Parser parser;
    auto data = fastgltf::GltfDataBuffer::FromPath(path);

    if (data.error() != fastgltf::Error::None) {
        throw_error("Failed to load asset {}\nError: {}",
                    absolute(path).string(),
                    getErrorMessage(data.error()));
    }
    constexpr auto options = fastgltf::Options::DecomposeNodeMatrices |
                             fastgltf::Options::LoadExternalBuffers |
                             fastgltf::Options::LoadExternalImages;

    auto result = parser.loadGltf(data.get(), path.parent_path(), options);
    if (result.error() != fastgltf::Error::None) {
        fmt::println(stderr,
                     "Failed to load asset {} \n{}",
                     filepath,
                     getErrorMessage(result.error()));
        throw std::runtime_error("Failed to load asset");
    }

    fastgltf::Asset asset = std::move(result.get());

    // Offsets for the newly loaded meshes / materials loaded into the asset
    // server

    size_t materialOffset = materials.size() * sizeof(MaterialConstants);
    size_t imageOffset = gltfImages.size();
    size_t samplerOffset = samplers.size();

    DescriptorAllocator descriptorAllocator{};
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
    };
    descriptorAllocator.init(device, 1024, sizes);

    load_images(asset);
    load_samplers(asset);
    load_materials(asset, imageOffset, samplerOffset);

    auto models = load_models(asset, materialOffset);
    AssetNode newAsset = load_nodes(asset, models);

    assets.push_back(std::move(newAsset));
    allocators.push_back(std::move(descriptorAllocator));

    return { .index = static_cast<uint32_t>(assets.size()) - 1 };
}

void
add_node_hierarchy_to_entity(const Entity entity,
                             const Handle<AssetNode> handle,
                             const AssetNode& node,
                             Resource<World>& world)
{
    // reuse global transform functions for decomposition
    auto transform = GlobalTransform{ node.localTransform };

    auto child = world->spawn(Position{ transform.get_translation() },
                              Rotation{ transform.get_rotation() },
                              Scale{ transform.get_scale() },
                              Handle<AssetNode>{ handle });

    world->add_child(entity, child.get());

    if (node.model.has_value()) {
        child.with_children([&](ChildBuilder parent) {
            auto& [meshes, materials] = node.model.value();
            // Add all meshes as child entities
            for (const auto& [mesh, material] :
                 std::ranges::views::zip(meshes, materials)) {
                parent.spawn(Handle<Mesh>{ .index = mesh },
                             Handle<Material>{ .index = material.value_or(0) });
            }
        });
    }

    for (auto& childNode : node.children) {
        add_node_hierarchy_to_entity(child.get(), handle, childNode, world);
    };
}

void
AssetServer::add_asset_to_entity(const Entity entity,
                                 const Handle<AssetNode> handle,
                                 Resource<World>& world) const
{
    const auto& node = assets[handle.index];

    add_node_hierarchy_to_entity(entity, handle, node, world);
}
Material
AssetServer::get_material(const Handle<Material> material) const
{
    return materials[material.index];
}

Mesh
AssetServer::get_mesh(const Handle<Mesh> mesh) const
{
    return meshes[mesh.index];
}

void
AssetServer::cleanup()
{
    vkDeviceWaitIdle(device);
    for (auto& sampler : samplers) {
        vkDestroySampler(device, sampler, nullptr);
    }

    for (auto& pools : allocators) {
        pools.destroy_pools();
    }
}

Handle<ImageAsset>
AssetServer::load_ktx(const std::string& filepath)
{
    ktxTexture* texture;

    const auto err = ktxTexture_CreateFromNamedFile(
        filepath.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

    if (err != KTX_SUCCESS) {
        auto temp =
            std::filesystem::absolute(std::filesystem::path(filepath)).string();
        throw_error("Failed to load skybox, KTX error: {}\nFilepath {}",
                    ktxErrorString(err),
                    std::move(temp));
    }

    VkExtent3D extent;
    extent.width = texture->baseWidth;
    extent.height = texture->baseHeight;
    extent.depth = texture->baseDepth;

    cmdCreateImgs.emplace_back(texture, extent, true, false);

    // I'll make this better when I refactor the asset server
    uint32_t index = loadedImages.size();

    for (const auto& cmd : cmdCreateImgs) {
        if (!cmd.gltf) {
            ++index;
        }
    }

    return { index - 1 };
}

AllocatedImage&
AssetServer::get_image_from_handle(const Handle<ImageAsset> handle)
{
    return loadedImages[handle.index];
}

void
AssetServer::register_pass(RenderGraph::PassBuilder& pass,
                           Resource<VulkanResourceManager>& manager)
{
    // Mega buffer stagings
    if (vertexBuffer.current_upload_size() != 0) {
        pass.create_buffer(Passes::vertexBufferStaging,
                           true,
                           vertexBuffer.current_upload_size(),
                           VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
                           VMA_MEMORY_USAGE_CPU_TO_GPU);
        pass.write_buffer(Passes::vertexBufferStaging,
                          VK_ACCESS_2_TRANSFER_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        pass.write_buffer(vertexBuffer.handle,
                          VK_ACCESS_TRANSFER_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    }

    if (indiceBuffer.current_upload_size() != 0) {
        pass.create_buffer(Passes::indiceBufferStaging,
                           true,
                           indiceBuffer.current_upload_size(),
                           VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
                           VMA_MEMORY_USAGE_CPU_TO_GPU);
        pass.write_buffer(Passes::indiceBufferStaging,
                          VK_ACCESS_2_TRANSFER_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        pass.write_buffer(indiceBuffer.handle,
                          VK_ACCESS_TRANSFER_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    }

    if (materialBuffer.current_upload_size() != 0) {
        pass.create_buffer(Passes::materialBufferStaging,
                           true,
                           materialBuffer.current_upload_size(),
                           VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
                           VMA_MEMORY_USAGE_CPU_TO_GPU);
        pass.write_buffer(Passes::materialBufferStaging,
                          VK_ACCESS_2_TRANSFER_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        pass.write_buffer(materialBuffer.handle,
                          VK_ACCESS_TRANSFER_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    }

    // Reserve more handles if need be
    if (cmdCreateImgs.size() > reservedBufferHandles.size()) {
        const uint32_t diff =
            cmdCreateImgs.size() - reservedBufferHandles.size();

        for (uint32_t i = 0; i < diff; i++) {
            reservedBufferHandles.emplace_back(
                Passes::HandleAllocator::allocate_buffer(
                    "asset_server_reserved"));
        }
    }

    // Create image and staging buffer
    // Write to both
    for (const auto& [cmd, staging] :
         std::views::zip(cmdCreateImgs, reservedBufferHandles)) {
        ImageRequirements req{ .format = VK_FORMAT_R8G8B8A8_UNORM,
                               .extent = cmd.extent,
                               .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                         VK_IMAGE_USAGE_SAMPLED_BIT,
                               .aspects = VK_IMAGE_ASPECT_COLOR_BIT };

        const auto handle =
            manager->get_handle_from_requirements(req, "asset_texture_img");

        uninitialisedImages.push_back(
            manager->get_resource_from_handle(handle));

        pass.create_buffer(
            staging,
            true,
            Utils::calculate_image_size(cmd.extent, VK_FORMAT_R8G8B8A8_UNORM),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);

        pass.write_buffer(staging,
                          VK_ACCESS_2_TRANSFER_WRITE_BIT,
                          VK_PIPELINE_STAGE_2_TRANSFER_BIT);

        pass.write_image(handle,
                         VK_ACCESS_2_TRANSFER_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    }
}

void
AssetServer::flush(ParallelResource<RenderGraph::PassServer>& passServer)
{

    // When we add pass, we declare that we allocate handles for all these
    // images, and declare that we're writing to them
    // For the staging buffers, we run .create_buffer() for
    // each image / mega buffer we're uploading.
    // This allows the graph to manage lifetime i.e. free after upload

    // At a later time I'll worry about de allocating assets

    VkCommandBuffer cmd;
    {
        auto server = passServer.write();
        cmd = server->get_cmd_buffer(Passes::uploadAssetsPass);
    }

    if (cmd == VK_NULL_HANDLE) {
        return;
    }

    const auto& server = passServer.illegal();

    // Transfer the image data
    for (const auto& [create, image, stagingHandle] : std::views::zip(
             cmdCreateImgs, uninitialisedImages, reservedBufferHandles)) {

        const auto& staging = server.get_resource(stagingHandle);

        if (create.gltf) {
            Utils::upload_image_asset(create.data, cmd, image, staging);

            gltfImages.push_back(image);

            free(create.data);
        } else {
            if (create.ktx) {
                const auto ptr = static_cast<ktxTexture*>(create.data);
                Utils::upload_image_asset(ptr, cmd, image, staging);

                loadedImages.push_back(image);

                ktxTexture_Destroy(ptr);
            }
        }
    }

    // Reset cmds
    cmdCreateImgs.clear();
    uninitialisedImages.clear();

    if (vertexBuffer.current_upload_size() != 0) {
        vertexBuffer.push_to_gpu(
            cmd, server.get_resource(Passes::vertexBufferStaging));
    }

    if (indiceBuffer.current_upload_size() != 0) {
        indiceBuffer.push_to_gpu(
            cmd, server.get_resource(Passes::indiceBufferStaging));
    }

    if (materialBuffer.current_upload_size() != 0) {
        materialBuffer.push_to_gpu(
            cmd, server.get_resource(Passes::materialBufferStaging));
    }
}

void
AssetServer::declare_scene_access(RenderGraph::PassBuilder& pass)
{
    // For now I am comfortable guaranteeing we access in the vertex & fragment
    // shaders only
    pass.read_buffer(vertexBuffer.handle,
                     VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT);
    pass.read_buffer(indiceBuffer.handle, VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT);
    pass.read_buffer(materialBuffer.handle,
                     VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

    // Scene data buffers. Managed elsewhere, but only filled with data of
    // actual renderables.
    pass.read_buffer(Passes::entityDataBuffer,
                     VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);
    pass.read_buffer(Passes::sceneDataBuffer,
                     VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT);

    // I'd like to restructure the asset server at a later stage
    // Perhaps splitting it into several pieces so it's easier to manage, while
    // maintaining the unified api
    // Maybe a generic image loading interface, which I can then reuse for gltfs
    // For the time being, I simply assume we'll be accessing all gltfs

    // It might be valuable to only add barriers for specific assets at a later
    // stage
    for (const auto& image : gltfImages) {
        pass.read_image(image.handle,
                        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    }
}

void
AssetServer::declare_access_to_images(
    RenderGraph::PassBuilder& pass,
    const std::vector<Handle<ImageAsset>>& images)
{
    throw_error("Unimplemented");
}
