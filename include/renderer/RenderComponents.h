#pragma once

#include <glm/vec3.hpp>

namespace Cel::Renderer {

struct NoDraw
{};

struct NoShadows
{};

struct DirectionalLight
{
    // Normalised vector
    glm::vec3 direction;
    glm::vec3 colour;
    float intensity;
};

struct PointLight
{
    glm::vec3 colour;
    float intensity;
};

}