#pragma once

#include "Hierarchy.h"
#include "ecs/System.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Cel {

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

    explicit Position(const glm::mat4& transform);
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

    explicit Rotation(const glm::mat4& transform);
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

    explicit Scale(const glm::mat4& transform);
};

struct GlobalTransform
{
    glm::mat4 transform;

    glm::vec3 GetTranslation();

    glm::quat GetRotation();

    glm::vec3 GetScale();

    void TransformPropagation(const GlobalTransform& parent,
                              const Position& localPosition,
                              const Rotation& localRotation,
                              const Scale& localScale);

    static glm::mat4 TransformFromLocal(const Position& localPosition,
                                        const Rotation& localRotation,
                                        const Scale& localScale);
};

void
HierarchyPropagation(
    Query<With<GlobalTransform,
               const Position,
               const Rotation,
               const Scale,
               const Children>,
          Without<Parent>>& rootParentQuery,
    Query<With<const Children>>& parentQuery,
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>>&
        childQuery);

}
