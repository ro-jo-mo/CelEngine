#pragma once

#include <variant>

namespace Cel::Renderer::Asset::GltfLabel {

struct Scene
{
    size_t i;
};
struct Mesh
{
    size_t i;
};
struct Material
{
    size_t i;
};
struct Node
{
    size_t i;
};

using Label = std::variant<Scene, Mesh, Material, Node>;

}