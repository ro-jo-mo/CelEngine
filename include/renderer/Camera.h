#pragma once
#include "ecs/System.h"

#include <glm/glm.hpp>

struct VkExtent2D;

namespace Cel {
struct GlobalTransform;
}

namespace Cel::Renderer {

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

    /**
     * @brief Default constructor is required for std::array
     * Please do not use
     */
    Camera() {}

    static Camera Camera2d();
    /**
     * Constructs a new perspective camera with the specified vertical
     * fov
     * @return New 3D camera object
     */
    static Camera Camera3d(float fov, float nearPlane, float farPlane);

    [[nodiscard]] glm::mat4 get_projection_matrix(VkExtent2D extent) const;

    [[nodiscard]] glm::mat4 get_view_matrix() const;

    float fov;
    float farPlane;
    float nearPlane;

  private:
    Camera(const float fov, const float nearPlane, const float farPlane)
        : fov(fov)
        , farPlane(farPlane)
        , nearPlane(nearPlane)
        , projection(Projection::Perspective) {};

    void update_view_matrix(const glm::mat4& transform);

    glm::mat4 viewMatrix = glm::mat4(1.0f);

    Projection projection;

    friend void camera_system(Query<With<Camera, GlobalTransform>>& cameras);
};

// Update view matrix, run after transform propagation
void
camera_system(Query<With<Camera, GlobalTransform>>& cameras);

}