#define STB_IMAGE_IMPLEMENTATION

#include "renderer/AssetServer.h"

#include "core/Error.h"
#include "renderer/VulkanUtils.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/glm.hpp>
#include <ktx.h>
#include <ranges>
#include <stb_image.h>

using namespace Cel::Renderer;
using namespace Cel;

void
AssetServer::create_defaults()
{
    // checkerboard image
    int32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    int32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));
    std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            pixels[y * 16 + x] = (x % 2) ^ (y % 2) ? magenta : black;
        }
    }

    // For now, we'll default to a checkerboard when no texture is assigned
    // However, this is a little silly for normals roughness etc
    AllocatedImage checkerboard =
        Utils::create_image(pixels.data(),
                           VkExtent3D{ 16, 16, 1 },
                           VK_FORMAT_R8G8B8A8_UNORM,
                           VK_IMAGE_USAGE_SAMPLED_BIT,
                           false,
                           "default_checkerboard_alloc",
                           context,
                           allocator,
                           immediate,
                           graphicsQueue);

    images.push_back(checkerboard);

    VkSampler defaultSampler;
    VkSamplerCreateInfo samplerInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
    };

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;

    vkCreateSampler(context.device, &samplerInfo, nullptr, &defaultSampler);
    samplers.push_back(defaultSampler);

    // Skybox
    std::vector<float> skyboxVertices = { -1.0, 1.0,  -1.0, -1.0, -1.0, -1.0,
                                          1.0,  -1.0, -1.0, 1.0,  1.0,  -1.0,
                                          -1.0, -1.0, 1.0,  -1.0, 1.0,  1.0,
                                          1.0,  -1.0, 1.0,  1.0,  1.0,  1.0 };

    std::vector<uint32_t> skyboxIndices = {
        0, 1, 2, 2, 3, 0, 4, 1, 0, 0, 5, 4, 2, 6, 7, 7, 3, 2,
        4, 5, 7, 7, 6, 4, 0, 3, 7, 7, 5, 0, 1, 4, 2, 2, 4, 6
    };

    skyboxCube = Utils::upload_mesh(skyboxIndices,
                                   skyboxVertices,
                                   context,
                                   allocator,
                                   immediate,
                                   graphicsQueue);

    set_skybox("../../assets/skybox.ktx2");
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
                newMesh.firstIndex =
                    indiceBuffer.upload_data(indices.data(),
                                            indices.size() * sizeof(uint32_t),
                                            1,
                                            context,
                                            allocator,
                                            immediate,
                                            graphicsQueue);
                newMesh.indexCount = indices.size();
                newMesh.vertexOffset =
                    verticeBuffer.upload_data(vertices.data(),
                                             vertices.size() * sizeof(Vertex),
                                             1,
                                             context,
                                             allocator,
                                             immediate,
                                             graphicsQueue);

                meshes.push_back(newMesh);
            }
        }
        models.push_back(newModel);
    }

    return models;
}

std::optional<AllocatedImage>
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
        return {};
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

    auto newImage = Utils::create_image(img,
                                       size,
                                       VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_IMAGE_USAGE_SAMPLED_BIT,
                                       false,
                                       "gltf_image_alloc",
                                       context,
                                       allocator,
                                       immediate,
                                       graphicsQueue);

    stbi_image_free(img);

    return newImage;
}

void
AssetServer::load_images(fastgltf::Asset& asset)
{
    for (auto& image : asset.images) {
        auto img = load_image(asset, image);
        if (img.has_value()) {
            images.push_back(img.value());
        } else {
            fmt::println(stderr,
                         "Failed to load an image ({}) in gltf asset",
                         image.name);
            images.push_back(images[0]);
        }
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
        vkCreateSampler(context.device, &sampler, nullptr, &newSampler);

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
            images[texture.imageIndex.value() + imageOffset].imageView;

        VkSampler sampler;
        if (texture.samplerIndex.has_value()) {
            sampler = samplers[texture.samplerIndex.value() + samplerOffset];
        } else {
            sampler = samplers[0];
        }

        return textureCache.add_texture(view, sampler);
    }
    return textureCache.add_texture(images[0].imageView, samplers[0]);
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

        // REWRITE:
        // A single mega buffer
        // Has handy function to upload data
        // likely best to store as an array and upload whole thing at once?
        // Padding / alignment is important

        // Lastly create a descriptor for this material

        materialList.push_back(constants);
    }

    const auto bufferOffset = materialBuffer.upload_data(
        materialList.data(),
        sizeof(MaterialConstants) * materialList.size(),
        1,
        context,
        allocator,
        immediate,
        graphicsQueue);

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
    size_t imageOffset = images.size();
    size_t samplerOffset = samplers.size();

    DescriptorAllocator descriptorAllocator{};
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
    };
    descriptorAllocator.init(context.device, 1024, sizes);

    load_images(asset);
    load_samplers(asset);
    load_materials(asset, imageOffset, samplerOffset);

    auto models = load_models(asset, materialOffset);
    AssetNode newAsset = load_nodes(asset, models);

    assets.push_back(std::move(newAsset));
    allocators.push_back(std::move(descriptorAllocator));

    return { .index = static_cast<uint32_t>(assets.size()) - 1 };
}

AllocatedImage
AssetServer::load_skybox_image(const char* filepath)
{
    ktxTexture* texture;

    const auto err = ktxTexture_CreateFromNamedFile(
        filepath, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

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
    extent.depth = 1;

    AllocatedImage skyboxImg = Utils::create_cube_map(texture,
                                                    VK_FORMAT_R8G8B8A8_UNORM,
                                                    "skybox_cubemap_alloc",
                                                    context,
                                                    allocator,
                                                    immediate,
                                                    graphicsQueue);
    ktxTexture_Destroy(texture);

    return skyboxImg;
}

void
AssetServer::set_skybox(const char* filepath)
{

    images.push_back(load_skybox_image(filepath));
    textureCache.add_texture(images[images.size() - 1].imageView, samplers[0]);
    skyboxTextureIndex = textureCache.descriptors.size() - 1;
}

void
AddNodeHierarchyToEntity(const Entity entity,
                         const AssetNode& node,
                         Resource<World>& world)
{
    // reuse global transform functions for decomposition
    auto transform = GlobalTransform{ node.localTransform };

    auto child = world->spawn(Position{ transform.get_translation() },
                              Rotation{ transform.get_rotation() },
                              Scale{ transform.get_scale() });

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
        AddNodeHierarchyToEntity(child.get(), childNode, world);
    };
}

void
AssetServer::add_asset_to_entity(const Entity entity,
                              const Handle<AssetNode> assetHandle,
                              Resource<World>& world) const
{
    const auto& node = assets[assetHandle.index];

    AddNodeHierarchyToEntity(entity, node, world);
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
    vkDeviceWaitIdle(context.device);
    for (auto& sampler : samplers) {
        vkDestroySampler(context.device, sampler, nullptr);
    }
    for (auto& image : images) {
        vmaDestroyImage(allocator, image.image, image.allocation);
        vkDestroyImageView(context.device, image.imageView, nullptr);
    }
    {
        const auto& buffer = skyboxCube;
        vmaDestroyBuffer(allocator,
                         buffer.vertexBuffer.buffer,
                         buffer.vertexBuffer.allocation);
        vmaDestroyBuffer(allocator,
                         buffer.indexBuffer.buffer,
                         buffer.indexBuffer.allocation);
    }

    for (auto& pools : allocators) {
        pools.destroy_pools();
    }

    indiceBuffer.cleanup(allocator);
    verticeBuffer.cleanup(allocator);
    materialBuffer.cleanup(allocator);
}
