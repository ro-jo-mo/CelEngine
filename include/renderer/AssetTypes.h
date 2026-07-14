#pragma once

#include <fastgltf/types.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>

namespace Cel::Renderer {

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

struct Model
{
    std::vector<uint32_t> meshes;
    std::vector<std::optional<uint32_t>> materials;
};

struct Mesh
{
    uint32_t firstIndex;
    uint32_t indexCount;
    int32_t vertexOffset;
};

struct AssetNode
{
    std::string name;
    std::vector<AssetNode> children;
    glm::mat4 localTransform;
    std::optional<Model> model;
};

// Handle<Mesh>, Handle<AssetNode> etc
template<typename T>
struct Handle
{
    uint32_t index;
};

}