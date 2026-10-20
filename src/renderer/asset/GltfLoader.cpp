#include "renderer/asset/GltfLoader.h"

#include "core/Error.h"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <ranges>
#include <stb_image.h>

void
Cel::Renderer::Asset::GltfLoader::prepare(std::string file,
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

    GltfData data{};

    load_meshes(asset, data);
    load_samplers(asset, data);
    load_materials(asset, data);
    load_images(asset, data);

    auto models = load_models(asset, materialOffset);
    SceneAsset newAsset = load_nodes(asset, models);
}

void
Cel::Renderer::Asset::GltfLoader::commit()
{
}

static VkFilter
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

static VkSamplerMipmapMode
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

        sampler.magFilter = ExtractFilter(
            gltfSampler.magFilter.value_or(fastgltf::Filter::Nearest));
        sampler.minFilter = ExtractFilter(
            gltfSampler.minFilter.value_or(fastgltf::Filter::Nearest));

        sampler.mipmapMode = ExtractMipMap(
            gltfSampler.minFilter.value_or(fastgltf::Filter::Nearest));

        VkSampler newSampler;
        vkCreateSampler(shared.device, &sampler, nullptr, &newSampler);

        data.samplers.push_back(newSampler);
    }
}

void
Cel::Renderer::Asset::GltfLoader::load_materials(fastgltf::Asset& asset,
                                                 GltfData& data)
{

    data.materials.reserve(asset.materials.size());
    for (auto& gltfMaterial : asset.materials) {
        // Material constants will be stored in a buffer on the gpu
        // The material will then contain the buffer index and offset for these
        // constants
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

        if (baseColorTexture.has_value()) {
            constants.colorTextureIndex = baseColorTexture.value().textureIndex;
        };

        if (metallicRoughnessTexture.has_value()) {
            constants.metalRoughnessTextureIndex =
                metallicRoughnessTexture.value().textureIndex;
        };

        if (gltfMaterial.normalTexture.has_value()) {
            constants.normalTextureIndex =
                gltfMaterial.normalTexture.value().textureIndex;
        }
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
