#pragma once

#include "renderer/Descriptors.h"
#include "renderer/resource-management/MegaBuffer.h"

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <optional>
#include <vector>

namespace Cel::Renderer::Asset {

// Indices into the global image and sampler descriptor arrays
struct TextureAsset
{
    uint32_t texture;
    uint32_t sampler;
};

struct ImageAsset
{
    AllocatedImage image;
    uint32_t mipLevels;
    uint32_t arrayLayers;
    VkImageViewType viewType;
};

struct MeshAsset
{};

struct Vertex
{
    glm::vec3 position;
    float uvX;
    glm::vec3 normal;
    float uvY;
};

// TODO probably replace this with handle. Rename MaterialConstants -> Material
// Index into the material constants buffer
struct Material
{
    uint32_t bufferIndex;
};

// Uploaded to gpu buffer, used through buffer reference
struct MaterialConstants
{
    glm::vec4 baseColorFactors;
    glm::vec4 metalRoughnessFactors;
    uint32_t colorTextureIndex;
    uint32_t metalRoughnessTextureIndex;
    uint32_t normalTextureIndex;
    uint32_t colorSamplerIndex;
    uint32_t metalRoughnessSamplerIndex;
    uint32_t normalSamplerIndex;
};

struct CpuPrimitive
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct Primitive
{
    uint32_t firstIndex;
    uint32_t indexCount;
    int32_t vertexOffset;
};

struct Mesh
{
    std::vector<Handle<Primitive>> primitives;
    // TODO This probably shouldn't be optional. Replace with some default
    // material
    std::vector<std::optional<uint32_t>> materials;
};

struct SceneAsset
{
    std::vector<SceneAsset> children;
    glm::mat4 localTransform;
    std::optional<Mesh> mesh;
};

struct SharedAssetData
{
    VkDevice device{};
    MegaBuffer vertexBuffer;
    MegaBuffer indexBuffer;
    MegaBuffer materialBuffer;
    TextureCache textures;
    SamplerCache samplers;
};

}