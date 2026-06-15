#pragma once
#include "ecs/System.h"

#include <glm/glm.hpp>

struct VkExtent2D;

namespace Cel {
struct GlobalTransform;
}

namespace Cel::Renderer {
class CameraSystem;

/**
 * @brief A camera component. Can render in 2D or 3D (Orthographic/Perspective)
 * I have not implemented orthographic yet
 * In a later stage, I might just split these into two components
 */
class Camera
{

  public:
    enum class Projection : char
    {
        Perspective,
        Orthographic
    };

    Camera() = delete;

    static Camera Camera2d();
    /**
     * Constructs a new perspective camera with the specified vertical
     * fov
     * @return New 3D camera object
     */
    static Camera Camera3d(float fov, float nearPlane, float farPlane);

    [[nodiscard]] glm::mat4 GetProjectionMatrix(VkExtent2D extent) const;

    glm::mat4 viewMatrix = glm::mat4(1.0f);

  private:
    Camera(const float fov, const float nearPlane, const float farPlane)
        : fov(fov)
        , farPlane(farPlane)
        , nearPlane(nearPlane)
        , projection(Projection::Perspective) {};

    void UpdateViewMatrix(const glm::mat4& transform);

    float fov;
    float farPlane;
    float nearPlane;
    Projection projection;

    friend class CameraSystem;
};

// Update view matrix, run after transform propagation
class CameraSystem : public System<Query<With<Camera, GlobalTransform>>>
{
  public:
    void Run(Query<With<Camera, GlobalTransform>>& cameras) override;
};
}