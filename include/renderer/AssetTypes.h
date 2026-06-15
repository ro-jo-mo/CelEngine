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

struct Material
{
    uint32_t bufferIndex;
    uint32_t bufferOffset;

    VkDescriptorSet materialSet;
};

struct MaterialConstants
{
    glm::vec4 baseColorFactors;
    glm::vec4 metalRoughnessFactors;
    // padding, we need it anyway for uniform buffers
    uint32_t colorTextureIndex;
    uint32_t metalRoughnessTextureIndex;
    uint32_t normalTextureIndex;
    uint32_t pad1;
    glm::vec4 extra[13];
};

struct Model
{
    std::vector<size_t> meshes;
    std::vector<std::optional<size_t>> materials;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
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
    size_t index;
};

}