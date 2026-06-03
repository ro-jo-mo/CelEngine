#include "core/Transform.h"

using namespace Cel;

glm::vec3
GlobalTransform::GetTranslation()
{
    return { transform[3] };
}

glm::quat
GlobalTransform::GetRotation()
{
    const glm::vec3 scale(glm::length(glm::vec3(transform[0])),
                          glm::length(glm::vec3(transform[1])),
                          glm::length(glm::vec3(transform[2])));

    glm::mat3 rotMat(glm::vec3(transform[0]) / scale.x,
                     glm::vec3(transform[1]) / scale.y,
                     glm::vec3(transform[2]) / scale.z);

    if (glm::determinant(rotMat) < 0.0f) {
        rotMat[2] = -rotMat[2];
    }

    return glm::quat_cast(rotMat);
}

glm::vec3
GlobalTransform::GetScale()
{

    glm::vec3 scale(glm::length(glm::vec3(transform[0])),
                    glm::length(glm::vec3(transform[1])),
                    glm::length(glm::vec3(transform[2])));

    const glm::mat3 rotMat(glm::vec3(transform[0]) / scale.x,
                           glm::vec3(transform[1]) / scale.y,
                           glm::vec3(transform[2]) / scale.z);

    if (glm::determinant(rotMat) < 0.0f) {
        scale.z = -scale.z;
    }

    return scale;
}

void
GlobalTransform::TransformPropagation(const GlobalTransform& parent,
                                      const Position& localPosition,
                                      const Rotation& localRotation,
                                      const Scale& localScale)
{
    // firstly construct a transform matrix from the local values
    const auto local =
        TransformFromLocal(localPosition, localRotation, localScale);
    // Global transform is equal to ParentGlobalTransform * LocalTransform
    transform = parent.transform * local;
}

glm::mat4
GlobalTransform::TransformFromLocal(const Position& localPosition,
                                    const Rotation& localRotation,
                                    const Scale& localScale)
{
    auto transform = glm::mat4_cast(localRotation.rotation);
    transform[0] *= localScale.scale.x;
    transform[1] *= localScale.scale.y;
    transform[2] *= localScale.scale.z;
    transform[3] = glm::vec4(localPosition.position, 1.0f);

    return transform;
}

void
PropagateThroughChildren(
    const Entity& parent,
    Query<With<const Children>>& parentQuery,
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>>&
        childQuery)
{
    // for child of this node
    // child transform = ...
    // recurse on child

    // Naturally we first check if the node even has kids
    // i.e. is this node a parent?

    if (!parentQuery.Has(parent)) {
        return;
    }

    const auto parentTransform = std::get<0>(childQuery.Get(parent));

    for (auto childId : std::get<0>(parentQuery.Get(parent)).children) {
        auto [transform, pos, rot, scale] = childQuery.Get(childId);
        transform.TransformPropagation(parentTransform, pos, rot, scale);

        PropagateThroughChildren(childId, parentQuery, childQuery);
    }
}

void
HierarchyPropagation::Run(
    Query<With<GlobalTransform,
               const Position,
               const Rotation,
               const Scale,
               const Children>,
          Without<Parent>>& rootParentQuery,
    Query<With<const Children>>& parentQuery,
    Query<With<GlobalTransform, const Position, const Rotation, const Scale>>&
        childQuery)
{
    // Parents represents the list of root nodes in the hierarchy trees
    // (i.e. an entity with children, but no parent)
    // For every node in the tree, we want to calculate their GlobalTransform
    // Then we can calculate the GlobalTransform of their children

    // At this point, there is no change detection
    // Every entity has its global transform recalculated each frame
    // This should be optimised once a form of change detection is implemented

    for (auto [parentTransform,
               parentPos,
               parentRot,
               parentScale,
               parentsChildren] : rootParentQuery) {
        parentTransform.transform = GlobalTransform::TransformFromLocal(
            parentPos, parentRot, parentScale);
        for (auto childId : parentsChildren.children) {
            auto [transform, pos, rot, scale] = childQuery.Get(childId);
            transform.TransformPropagation(parentTransform, pos, rot, scale);

            PropagateThroughChildren(childId, parentQuery, childQuery);
        }
    }
}
