#include "renderer/Camera.h"

#include "core/Error.h"
#include "core/Transform.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <vulkan/vulkan.h>

Cel::Renderer::Camera
Cel::Renderer::Camera::Camera2d()
{
    throw_error(("Unimplemented!"));
    return Camera3d(0, 0, 0);
}

Cel::Renderer::Camera
Cel::Renderer::Camera::Camera3d(const float fov,
                                const float nearPlane,
                                const float farPlane)
{
    return { fov, nearPlane, farPlane };
}

glm::mat4
Cel::Renderer::Camera::get_view_matrix() const
{
    return viewMatrix;
}

void
Cel::Renderer::Camera::update_view_matrix(const glm::mat4& transform)
{
    // Extract translation
    const auto position = glm::vec3(transform[3]);

    // Extract and normalize the basis vectors to remove scale
    const glm::vec3 right = glm::normalize(glm::vec3(transform[0]));
    const glm::vec3 up = glm::normalize(glm::vec3(transform[1]));
    const glm::vec3 forward = glm::normalize(glm::vec3(transform[2]));

    // Reconstruct a pure rotation+translation matrix
    glm::mat4 cleanTransform(1.0f);
    cleanTransform[0] = glm::vec4(right, 0.0f);
    cleanTransform[1] = glm::vec4(up, 0.0f);
    cleanTransform[2] = glm::vec4(forward, 0.0f);
    cleanTransform[3] = glm::vec4(position, 1.0f);

    viewMatrix = glm::inverse(cleanTransform);
}

glm::mat4
Cel::Renderer::Camera::get_projection_matrix(const VkExtent2D extent) const
{
    const float aspect =
        static_cast<float>(extent.width) / static_cast<float>(extent.height);
    return glm::perspective(fov, aspect, farPlane, nearPlane);
}

void
Cel::Renderer::camera_system(Query<With<Camera, GlobalTransform>>& cameras)
{
    for (auto [camera, transform] : cameras) {
        camera.update_view_matrix(transform.transform);
    }
}
