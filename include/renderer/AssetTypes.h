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

struct AssetNode
{
    std::string name;
    std::vector<AssetNode> children;
    glm::mat4 localTransform;
    std::optional<size_t> model;
};

struct Primitive
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::optional<size_t> materialIndex;
};

struct Model
{
    std::vector<Primitive> primitives;
};

// Stores the asset structure
struct SceneAsset
{
    AssetNode root;
    std::vector<Material> materials;
    std::vector<AllocatedImage> images;
    std::vector<Model> models;
};

// Handle<Mesh>, Handle<AssetNode> etc
template<typename T>
struct Handle
{
    size_t index;
};

}