#pragma once

#include <fastgltf/types.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Cel::Renderer::Asset {

struct ImageAsset
{};

struct MeshAsset
{};

struct Vertex
{
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
};

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
    VkDevice device;
    MegaBuffer vertexBuffer;
    MegaBuffer indexBuffer;
    MegaBuffer materialBuffer;
    TextureCache textures;
};

}