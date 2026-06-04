#pragma once

#include <fastgltf/types.hpp>

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
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;

    std::optional<int> baseColorTexture;
    std::optional<int> metallicRoughnessTexture;
    std::optional<int> normalTexture;

    bool doubleSided;
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