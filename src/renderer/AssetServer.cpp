#define STB_IMAGE_IMPLEMENTATION

#include "renderer/AssetServer.h"

#include "renderer/VulkanUtils.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <ranges>
#include <stb_image.h>

using namespace Cel::Renderer;
using namespace Cel;

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
                meshes.push_back(newMesh);
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

void
AssetServer::LoadMaterials(fastgltf::Asset& asset, size_t imageOffset)
{
    for (auto& gltfMaterial : asset.materials) {
        Material newMaterial{};

        newMaterial.doubleSided = gltfMaterial.doubleSided;

        const auto& pbr = gltfMaterial.pbrData;
        auto& colour = pbr.baseColorFactor;
        newMaterial.baseColorFactor = {
            colour.x(), colour.y(), colour.z(), colour.w()
        };
        newMaterial.roughnessFactor = pbr.roughnessFactor;
        newMaterial.metallicFactor = pbr.metallicFactor;

        if (gltfMaterial.normalTexture.has_value()) {
            newMaterial.normalTexture =
                gltfMaterial.normalTexture.value().textureIndex + imageOffset;
        }
        if (pbr.metallicRoughnessTexture.has_value()) {
            newMaterial.metallicRoughnessTexture =
                pbr.metallicRoughnessTexture.value().textureIndex + imageOffset;
        }
        if (pbr.baseColorTexture.has_value()) {
            newMaterial.baseColorTexture =
                pbr.baseColorTexture.value().textureIndex + imageOffset;
        }

        materials.push_back(newMaterial);
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
        newNode.model = std::move(models[node.meshIndex.value()]);
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

    LoadImages(asset);
    LoadMaterials(asset, imageOffset);
    LoadModels(asset, materialOffset);
    auto models = LoadModels(asset, materialOffset);
    AssetNode newAsset = LoadNodes(asset, models);

    assets.push_back(std::move(newAsset));

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