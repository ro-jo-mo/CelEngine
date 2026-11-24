#pragma once

#include <glm/common.hpp>
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
    // Gpu representation
    struct Data;

    glm::vec3 colour;
    float intensity;
};

struct PointLight::Data
{
    PointLight light;
    glm::vec3 position;
};

}