#include "renderer/asset/GltfLoader.h"

#include "core/Error.h"
#include "renderer/resource-management/VulkanResourceManager.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <mutex>
#include <ranges>
#include <stb_image.h>

void
Cel::Renderer::Asset::GltfLoader::prepare(Handle<SceneAsset> handle,
                                          std::string file,
                                          GltfLabel::Scene label)
{
    std::filesystem::path path = file;

    fastgltf::Asset asset;
    {
        fastgltf::Parser parser;

        auto gltfData = fastgltf::GltfDataBuffer::FromPath(path);

        if (gltfData.error() != fastgltf::Error::None) {
            throw_error("Failed to load asset {}\nError: {}",
                        absolute(path).string(),
                        getErrorMessage(gltfData.error()));
        }
        constexpr auto options = fastgltf::Options::DecomposeNodeMatrices |
                                 fastgltf::Options::LoadExternalBuffers |
                                 fastgltf::Options::LoadExternalImages;

        auto result =
            parser.loadGltf(gltfData.get(), path.parent_path(), options);
        if (result.error() != fastgltf::Error::None) {
            throw_error("Failed to load asset {} \n{}",
                        file.c_str(),
                        getErrorMessage(result.error()));
        }

        asset = std::move(result.get());
    }

    GltfData data{ .handle = handle };

    load_meshes(asset, data);
    load_samplers(asset, data);
    load_materials(asset, data);
    load_images(asset, data);
    load_nodes(asset, data);

    std::unique_lock guard{ processingLock };
    processing.push_back(data);
}

void
Cel::Renderer::Asset::GltfLoader::flush_cpu(VulkanResourceManager& manager,
                                            SharedAssetData& shared)
{
    std::unique_lock guard{ processingLock };

    for (auto& data : processing) {

        data.imageOffset = images.size();
        data.samplerOffset = samplers.size();
        data.primitiveOffset = primitives.size();
        shared.commit(data, manager, shared);
    }

    // DON'T CLEAR HERE. WAIT UNTIL PASS FOR GPU PUSH
    processing.clear();
}

void
Cel::Renderer::Asset::GltfLoader::commit(GltfData& data,
                                         uint32_t imageOffset,
                                         uint32_t samplerOffset,
                                         uint32_t primitiveOffset,
                                         uint32_t materialOffset,
                                         VulkanResourceManager& manager,
                                         SharedAssetData& shared)
{
    for (const auto& [_, extent] : data.images) {

        ImageRequirements requirements{ .format = VK_FORMAT_R8G8B8A8_UNORM,
                                        .extent = extent,
                                        .usages =
                                            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                            VK_IMAGE_USAGE_SAMPLED_BIT,
                                        .aspects = VK_IMAGE_ASPECT_COLOR_BIT };

        const auto handle =
            manager.get_handle_from_requirements(requirements, "gltf_image");

        images.push_back(manager.get_resource_from_handle(handle));
    }

    // We only really store samplers to delete later
    // If we delayed creation to now we could de duplicate
    samplers.append_range(data.samplers);

    auto add_texture = [&](auto& pair, auto& set, uint32_t defaultImg) {
        auto [img, sampler] = pair;

        if (img == UINT32_MAX) {
            img = defaultImg;
        } else {
            img += imageOffset;
        }

        if (sampler == UINT32_MAX) {
            sampler = 0;
        } else {
            sampler += samplerOffset;
        }

        set = shared.textures.add_texture(images[img].imageView,
                                          samplers[sampler]);
    };

    // Set the index into texture array for materials
    for (const auto& [i, material] : std::views::enumerate(data.materials)) {
        add_texture(data.textureSamplers[i], material.colorTextureIndex, 0);

        add_texture(data.textureSamplers[i + 1],
                    material.metalRoughnessTextureIndex,
                    0);

        add_texture(
            data.textureSamplers[i + 2], material.normalTextureIndex, 0);
    }

    shared.materialBuffer.allocate(data.materials.data(),
                                   data.materials.size() *
                                       sizeof(MaterialConstants));

    // Convert the cpu mesh data to offsets in the gpu buffers
    for (const auto& cpuPrim : data.primitives) {
        auto& primitive = primitives.emplace_back();

        primitive.firstIndex = shared.indexBuffer.allocate(
            cpuPrim.indices.data(), cpuPrim.indices.size() * sizeof(uint32_t));

        primitive.indexCount = cpuPrim.indices.size();

        primitive.vertexOffset = shared.vertexBuffer.allocate(
            cpuPrim.vertices.data(), cpuPrim.vertices.size() * sizeof(Vertex));
    }

    // Remap meshes to committed primitives
    auto remap_tree = [&](SceneAsset& scene, auto&& _remap_tree) -> void {
        if (scene.mesh.has_value()) {
            auto& mesh = scene.mesh.value();

            for (const auto& [prim, mat] :
                 std::views::zip(mesh.primitives, mesh.materials)) {
                prim.index += primitiveOffset;

                if (mat.has_value()) {
                    mat = mat.value() + materialOffset;
                }
            }
        }

        for (auto& child : scene.children) {
            _remap_tree(child, _remap_tree);
        }
    };

    remap_tree(data.scene, remap_tree);

    scenes[data.handle.index] = data.scene;
}

static VkFilter
extract_filter(const fastgltf::Filter filter)
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

static VkSamplerMipmapMode
extract_mip_map(const fastgltf::Filter filter)
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
Cel::Renderer::Asset::GltfLoader::load_samplers(const fastgltf::Asset& asset,
                                                GltfData& data) const
{
    data.samplers.reserve(asset.samplers.size());
    for (auto& gltfSampler : asset.samplers) {
        VkSamplerCreateInfo sampler = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr
        };
        sampler.maxLod = VK_LOD_CLAMP_NONE;
        sampler.minLod = 0;

        sampler.magFilter = extract_filter(
            gltfSampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampler.minFilter = extract_filter(
            gltfSampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampler.mipmapMode = extract_mip_map(
            gltfSampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler;
        vkCreateSampler(device, &sampler, nullptr, &newSampler);

        data.samplers.push_back(newSampler);
    }
}

void
Cel::Renderer::Asset::GltfLoader::load_materials(fastgltf::Asset& asset,
                                                 GltfData& data)
{

    data.materials.reserve(asset.materials.size());
    data.textureSamplers.reserve(asset.materials.size() * 3);

    auto resolve = [&](const auto& textInfo) {
        // While 0 represents the index into samplers for the default, as we +
        // offset these values, we need a flag

        auto& [image, sampler] =
            data.textureSamplers.emplace_back(UINT32_MAX, UINT32_MAX);

        if (textInfo.has_value()) {
            auto& texture = asset.textures[textInfo.value().textureIndex];

            image = texture.imageIndex.value();

            if (texture.samplerIndex.has_value()) {
                sampler = texture.samplerIndex.value();
            }
        }
    };

    for (auto& gltfMaterial : asset.materials) {

        auto& constants = data.materials.emplace_back();

        const auto& [baseColorFactor,
                     metallicFactor,
                     roughnessFactor,
                     baseColorTexture,
                     metallicRoughnessTexture] = gltfMaterial.pbrData;

        constants.baseColorFactors = { baseColorFactor.x(),
                                       baseColorFactor.y(),
                                       baseColorFactor.z(),
                                       baseColorFactor.w() };

        constants.metalRoughnessFactors = {
            metallicFactor, roughnessFactor, 0, 0
        };

        resolve(baseColorTexture);

        resolve(metallicRoughnessTexture);

        resolve(gltfMaterial.normalTexture);
    }
}

void
Cel::Renderer::Asset::GltfLoader::load_meshes(fastgltf::Asset& asset,
                                              GltfData& data)
{

    for (const auto& gltfMesh : asset.meshes) {
        Mesh& newMesh = data.meshes.emplace_back();

        for (const auto& [i, gltfPrimitive] :
             std::views::enumerate(gltfMesh.primitives)) {

            newMesh.primitives.push_back(i);
            auto& [vertices, indices] = data.primitives.emplace_back();

            // Associate the new mesh with a material
            fastgltf::Optional<uint32_t> materialIndex{};
            if (materialIndex.has_value()) {
                materialIndex =
                    materialIndex.value() * sizeof(MaterialConstants);
            }

            newMesh.materials.push_back(materialIndex);

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
            }
        }
    }
}

void
Cel::Renderer::Asset::GltfLoader::load_images(fastgltf::Asset& asset,
                                              GltfData& data)
{
    for (auto& gltfImage : asset.images) {
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
                    auto& data =
                        std::get<fastgltf::sources::Array>(buffer.data);
                    auto begin = data.bytes.begin() + view.byteOffset;
                    imageData.assign(begin, begin + view.byteLength);
                },
                [&](const fastgltf::sources::URI&) { ErrorMsg("URI"); },
                [&](const fastgltf::sources::Vector&) { ErrorMsg("Vector"); },
                [&](const std::monostate&) { ErrorMsg("Monostate"); },
                [&](const fastgltf::sources::CustomBuffer&) {
                    ErrorMsg("CustomBuffer");
                },
                [&](const fastgltf::sources::ByteView&) {
                    ErrorMsg("ByteView");
                },
                [&](const fastgltf::sources::Fallback&) {
                    ErrorMsg("Fallback");
                } },
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

        data.images.emplace_back(img, size);
    }
}

void
Cel::Renderer::Asset::GltfLoader::load_nodes(fastgltf::Asset& asset,
                                             GltfData& data)
{
    auto& scene = asset.scenes[asset.defaultScene.value_or(0)];

    std::vector<SceneAsset> roots;
    roots.reserve(scene.nodeIndices.size());

    for (const auto& index : scene.nodeIndices) {
        roots.push_back(load_scene_tree(index, asset, data));
    }

    std::vector<size_t> rootIds(roots.size());

    SceneAsset root = { .children = roots, .localTransform = glm::mat4(1.0f) };

    data.scene = { .children = roots, .localTransform = glm::mat4x4(1.0f) };
}

Cel::Renderer::Asset::SceneAsset
Cel::Renderer::Asset::GltfLoader::load_scene_tree(const size_t nodeIndex,
                                                  fastgltf::Asset& asset,
                                                  GltfData& data)
{
    auto node = asset.nodes[nodeIndex];

    SceneAsset newNode{};
    newNode.children.reserve(node.children.size());

    if (node.meshIndex.has_value()) {
        newNode.mesh = data.meshes[node.meshIndex.value()];
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
        newNode.children.push_back(load_scene_tree(child, asset, data));
    }

    return newNode;
}
