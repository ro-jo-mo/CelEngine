#pragma once

#include "Hierarchy.h"
#include "ecs/System.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Cel {

struct Position;
struct Rotation;
struct Scale;

struct GlobalTransform
{
    glm::mat4 transform;

    glm::vec3 get_translation();

    glm::quat get_rotation();

    glm::vec3 get_scale();

    void transform_propagation(const GlobalTransform& parent,
                              const Position& localPosition,
                              const Rotation& localRotation,
                              const Scale& localScale);

    static glm::mat4 transform_from_local(const Position& localPosition,
                                        const Rotation& localRotation,
                                        const Scale& localScale);
};

// I decided to use position over translation to conform with Unity
struct Position
{
    glm::vec3 position;

    Position()
        : position(0.0f)
    {
    }

    explicit Position(const glm::vec3 v)
        : position(v)
    {
    }

    Position(const float x, const float y, const float z)
        : position(x, y, z)
    {
    }

    explicit Position(const glm::mat4& transform)
        : position(GlobalTransform{ transform }.get_translation())
    {
    }
};

struct Rotation
{
    glm::quat rotation;
    // identity quaternion
    Rotation()
        : rotation(1.0f, 0.0f, 0.0f, 0.0f)
    {
    }

    explicit Rotation(const glm::quat quat)
        : rotation(quat)
    {
    }

    explicit Rotation(const glm::vec3 eulerRadians)
        : rotation(eulerRadians)
    {
    }

    Rotation(const float pitch, const float yaw, const float roll)
        : rotation(glm::vec3(pitch, yaw, roll))
    {
    }

    explicit Rotation(const glm::mat4& transform)
        : rotation(GlobalTransform{ transform }.get_rotation())
    {
    }
};

struct Scale
{
    glm::vec3 scale;

    Scale()
        : scale(1.0f)
    {
    }

    explicit Scale(const glm::vec3 v)
        : scale(v)
    {
    }

    Scale(const float x, const float y, const float z)
        : scale(x, y, z)
    {
    }

    explicit Scale(const float uniform)
        : scale(uniform)
    {
    }

    explicit Scale(const glm::mat4& transform)
        : scale(GlobalTransform{ transform }.get_scale())
    {
    }
};

void
hierarchy_propagation(
    Query<With<const GlobalTransform, const Children>, Without<Parent>>&
        rootQuery,
    Query<With<const Children>>& parentQuery,
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>>&
        childQuery);

void
compute_root_global_transform(
    Query<With<GlobalTransform, Position, Rotation, Scale>, Without<Parent>>&
        rootQuery);

}
