#define STB_IMAGE_IMPLEMENTATION

#include "renderer/AssetServer.h"

#include "renderer/VulkanUtils.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <glm/glm.hpp>
#include <ranges>
#include <stb_image.h>

using namespace Cel::Renderer;
using namespace Cel;

void
AssetServer::CreateDefaults()
{
    // checkerboard image
    int32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    int32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 1));
    std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }

    // For now, we'll default to a checkerboard when no texture is assigned
    // However, this is a little silly for normals roughness etc
    AllocatedImage checkerboard = Utils::CreateImage(pixels.data(),
                                                     VkExtent3D{ 16, 16, 1 },
                                                     VK_FORMAT_R8G8B8A8_UNORM,
                                                     VK_IMAGE_USAGE_SAMPLED_BIT,
                                                     false,
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
}

std::vector<Model>
AssetServer::LoadModels(fastgltf::Asset& asset, size_t materialOffset)
{
    std::vector<Model> models;

    for (const auto& gltfMesh : asset.meshes) {
        Model newModel;

        for (const auto& gltfPrimitive : gltfMesh.primitives) {
            Mesh newMesh;

            // Associate the new mesh with a material
            auto materialIndex = gltfPrimitive.materialIndex;
            if (materialIndex.has_value()) {
                materialIndex = materialIndex.value() + materialOffset;
            }
            newModel.materials.push_back(materialIndex);

            // Load indices
            {
                fastgltf::Accessor& accessor =
                    asset.accessors[gltfPrimitive.indicesAccessor.value()];

                newMesh.indices.reserve(accessor.count);

                fastgltf::iterateAccessor<uint32_t>(
                    asset, accessor, [&](const uint32_t idx) {
                        newMesh.indices.push_back(idx);
                    });
            }

            // Load vertices
            {
                fastgltf::Accessor& accessor =
                    asset.accessors[gltfPrimitive.findAttribute("POSITION")
                                        ->accessorIndex];

                newMesh.vertices.reserve(accessor.count);

                fastgltf::iterateAccessor<fastgltf::math::fvec3>(
                    asset, accessor, [&](fastgltf::math::fvec3 vert) {
                        Vertex newVertex;
                        newVertex.position = { vert.x(), vert.y(), vert.z() };
                        newVertex.normal = { 1, 0, 0 };
                        newVertex.uv_x = 0;
                        newVertex.uv_y = 0;
                        newMesh.vertices.push_back(newVertex);
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
                            newMesh.vertices[index].normal = { normal.x(),
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
                            newMesh.vertices[index].uv_x = vert.x();
                            newMesh.vertices[index].uv_y = vert.y();
                        });
                }

                newModel.meshes.push_back(meshes.size());
                // For now store both cpu & gpu
                meshes.push_back(newMesh);
                meshBuffers.push_back(Utils::UploadMesh(newMesh.indices,
                                                        newMesh.vertices,
                                                        context,
                                                        allocator,
                                                        immediate,
                                                        graphicsQueue));
            }
        }
        models.push_back(newModel);
    }

    return models;
}

std::optional<AllocatedImage>
AssetServer::LoadImage(fastgltf::Asset& asset, fastgltf::Image& gltfImage)
{

    std::vector<std::byte> imageData;

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
            [&](auto& image) {
                fmt::println(stderr, "Unexpected type in load gltf image");
                throw std::runtime_error("Unexpected type in load gltf image");
            },
        },
        gltfImage.data);

    if (imageData.size() == 0) {
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

    auto newImage = Utils::CreateImage(img,
                                       size,
                                       VK_FORMAT_R8G8B8A8_UNORM,
                                       VK_IMAGE_USAGE_SAMPLED_BIT,
                                       false,
                                       context,
                                       allocator,
                                       immediate,
                                       graphicsQueue);

    stbi_image_free(img);

    return newImage;
}

void
AssetServer::LoadImages(fastgltf::Asset& asset)
{
    for (auto& image : asset.images) {
        auto img = LoadImage(asset, image);
        if (img.has_value()) {
            images.push_back(img.value());
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
AssetServer::LoadSamplers(const fastgltf::Asset& asset)
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

void
AssetServer::WriteMaterialDescriptors(Material& material,
                                      DescriptorAllocator& descriptorAllocator)
{
    material.materialSet =
        descriptorAllocator.Allocate(globalDescriptorData.materialLayout);

    descriptorWriter.Clear();

    descriptorWriter.WriteBuffer(0,
                                 materialBuffers[material.bufferIndex].buffer,
                                 sizeof(MaterialConstants),
                                 material.bufferOffset,
                                 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    descriptorWriter.UpdateSet(context.device, material.materialSet);
}

uint32_t
AssetServer::ResolveTextureSampler(
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

        return textureCache.AddTexture(view, sampler);
    }
    return textureCache.AddTexture(images[0].imageView, samplers[0]);
}

void
AssetServer::LoadMaterials(fastgltf::Asset& asset,
                           DescriptorAllocator& descriptorAllocator,
                           const size_t imageOffset,
                           const size_t samplerOffset)
{
    const uint32_t bufferIndex = materials.size();
    uint32_t bufferOffset = 0;
    materialBuffers.push_back(
        Utils::CreateBuffer(asset.materials.size() * sizeof(MaterialConstants),
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VMA_MEMORY_USAGE_CPU_TO_GPU,
                            allocator));

    const auto buffer = static_cast<MaterialConstants*>(
        materialBuffers[bufferIndex].info.pMappedData);
    for (auto& gltfMaterial : asset.materials) {
        // Material constants will be stored in a buffer on the gpu
        // The material will then contain the buffer index and offset for these
        // constants
        MaterialConstants constants{};

        const auto& pbr = gltfMaterial.pbrData;
        auto& colour = pbr.baseColorFactor;
        constants.baseColorFactors = {
            colour.x(), colour.y(), colour.z(), colour.w()
        };
        constants.metalRoughnessFactors = {
            pbr.metallicFactor, pbr.roughnessFactor, 0, 0
        };

        constants.colorTextureIndex = ResolveTextureSampler(
            asset, pbr.baseColorTexture, imageOffset, samplerOffset);
        constants.metalRoughnessTextureIndex = ResolveTextureSampler(
            asset, pbr.metallicRoughnessTexture, imageOffset, samplerOffset);
        constants.normalTextureIndex = ResolveTextureSampler(
            asset,
            gltfMaterial.normalTexture.transform([](const auto& info) {
                return fastgltf::TextureInfo{ .textureIndex = info.textureIndex,
                                              .texCoordIndex =
                                                  info.texCoordIndex };
            }),
            imageOffset,
            samplerOffset);

        // Upload constants to buffer
        buffer[bufferOffset] = constants;

        Material newMaterial{
            .bufferIndex = bufferIndex,
            .bufferOffset = bufferOffset,
        };

        // Lastly create a descriptor for this material
        WriteMaterialDescriptors(newMaterial, descriptorAllocator);

        materials.push_back(newMaterial);
        ++bufferOffset;
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
AssetServer::LoadNodes(fastgltf::Asset& asset, std::vector<Model>& models)
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
AssetServer::LoadAsset(const char* filepath)
{
    std::filesystem::path path = filepath;

    fastgltf::Parser parser;
    auto data = fastgltf::GltfDataBuffer::FromPath(path);

    if (data.error() != fastgltf::Error::None) {
        fmt::println(stderr, "Failed to load asset {}", filepath);
        throw std::runtime_error("Failed to load asset");
    }
    constexpr auto options = fastgltf::Options::DecomposeNodeMatrices |
                             fastgltf::Options::LoadExternalBuffers |
                             fastgltf::Options::LoadExternalImages;

    auto result = parser.loadGltf(data.get(), path.parent_path(), options);
    if (result.error() != fastgltf::Error::None) {
        fmt::println(stderr, "Failed to load asset {}", filepath);
        throw std::runtime_error("Failed to load asset");
    }

    fastgltf::Asset asset = std::move(result.get());

    // Offsets for the newly loaded meshes / materials loaded into the asset
    // server

    size_t materialOffset = materials.size();
    size_t imageOffset = images.size();
    size_t samplerOffset = samplers.size();

    DescriptorAllocator descriptorAllocator{};
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
    };
    descriptorAllocator.Init(context.device, 1024, sizes);

    LoadImages(asset);
    LoadMaterials(asset, descriptorAllocator, imageOffset, samplerOffset);

    auto models = LoadModels(asset, materialOffset);
    AssetNode newAsset = LoadNodes(asset, models);

    assets.push_back(std::move(newAsset));
    allocators.push_back(std::move(descriptorAllocator));

    return { .index = assets.size() - 1 };
}

void
AddNodeHierarchyToEntity(const Entity entity,
                         const AssetNode& node,
                         Resource<World>& world)
{
    // reuse global transform functions for decomposition
    auto transform = GlobalTransform{ node.localTransform };

    auto child = world->Spawn(Position{ transform.GetTranslation() },
                              Rotation{ transform.GetRotation() },
                              Scale{ transform.GetScale() });

    world->AddChild(entity, child.Get());

    if (node.model.has_value()) {
        child.WithChildren([&](ChildBuilder parent) {
            auto& [meshes, materials] = node.model.value();
            // Add all meshes as child entities
            for (const auto& [mesh, material] :
                 std::ranges::views::zip(meshes, materials)) {
                parent.Spawn(Handle<Mesh>{ .index = mesh },
                             Handle<Material>{ .index = material.value_or(0) });
            }
        });
    }

    for (auto& childNode : node.children) {
        AddNodeHierarchyToEntity(child.Get(), childNode, world);
    };
}

void
AssetServer::AddAssetToEntity(const Entity entity,
                              const Handle<AssetNode> assetHandle,
                              Resource<World>& world) const
{
    const auto& node = assets[assetHandle.index];

    AddNodeHierarchyToEntity(entity, node, world);
}
Material
AssetServer::GetMaterial(const Handle<Material> material) const
{
    return materials[material.index];
}

const AllocatedMeshBuffer&
AssetServer::GetMesh(const Handle<Mesh> mesh) const
{
    return meshBuffers[mesh.index];
}

void
AssetServer::Cleanup()
{
    vkDeviceWaitIdle(context.device);
    for (auto& sampler : samplers) {
        vkDestroySampler(context.device, sampler, nullptr);
    }
    for (auto& image : images) {
        vmaDestroyImage(allocator, image.image, image.allocation);
        vkDestroyImageView(context.device, image.imageView, nullptr);
    }
    for (auto& buffer : meshBuffers) {
        vmaDestroyBuffer(allocator,
                         buffer.vertexBuffer.buffer,
                         buffer.vertexBuffer.allocation);
        vmaDestroyBuffer(allocator,
                         buffer.indexBuffer.buffer,
                         buffer.indexBuffer.allocation);
    }
    for (auto& pools : allocators) {
        pools.DestroyPools();
    }
}
