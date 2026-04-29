#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace Cel {
    struct Position {
        glm::vec3 position;

        Position() : position(0.0f) {
        }

        explicit Position(const glm::vec3 v) : position(v) {
        }

        Position(const float x, const float y, const float z) : position(x, y, z) {
        }
    };


    struct Rotation {
        glm::quat rotation;

        Rotation() : rotation(1.0f, 0.0f, 0.0f, 0.0f) {
        } // identity quaternion

        explicit Rotation(const glm::quat quat) : rotation(quat) {
        }

        explicit Rotation(const glm::vec3 eulerRadians) : rotation(eulerRadians) {
        }

        Rotation(const float pitch, const float yaw, const float roll) : rotation(glm::vec3(pitch, yaw, roll)) {
        }
    };


    struct Scale {
        glm::vec3 scale;

        Scale() : scale(1.0f) {
        }

        explicit Scale(const glm::vec3 v) : scale(v) {
        }

        Scale(const float x, const float y, const float z) : scale(x, y, z) {
        }

        Scale(const float uniform) : scale(uniform) {
        }
    };

    struct LocalPosition {
        glm::vec3 position;

        LocalPosition() : position(0.0f) {
        }

        explicit LocalPosition(const glm::vec3 v) : position(v) {
        }

        LocalPosition(const float x, const float y, const float z) : position(x, y, z) {
        }
    };


    struct LocalRotation {
        glm::quat rotation;

        LocalRotation() : rotation(1.0f, 0.0f, 0.0f, 0.0f) {
        } // identity quaternion

        explicit LocalRotation(const glm::quat quat) : rotation(quat) {
        }

        explicit LocalRotation(const glm::vec3 eulerRadians) : rotation(eulerRadians) {
        }

        LocalRotation(const float pitch, const float yaw, const float roll) : rotation(glm::vec3(pitch, yaw, roll)) {
        }
    };


    struct LocalScale {
        glm::vec3 scale;

        LocalScale() : scale(1.0f) {
        }

        explicit LocalScale(const glm::vec3 v) : scale(v) {
        }

        LocalScale(const float x, const float y, const float z) : scale(x, y, z) {
        }

        LocalScale(const float uniform) : scale(uniform) {
        }
    };

    glm::mat4x4 ConstructTransformMatrix(Position position, Rotation rotation, Scale scale);
}
