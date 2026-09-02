#pragma once

#include "ecs/Types.h"
#include "resource-management/MegaBuffer.h"

namespace Cel::Renderer::Passes {

// A resource containing data about the scene
struct SceneData
{
    // Struct that we'll push to the gpu
    struct
    {
        VkDeviceAddress verticesBufferAddress;
        VkDeviceAddress materialBufferAddress;
        VkDeviceAddress perEntityBufferAddress;
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        glm::mat4 viewProjMatrix;
    } data;

    PerFrameMegaBuffer entityBuffer;

    /**
     * Contains a mapping of entity id to the entity buffer.
     * Why is it here? When setting the "firstInstance" value on indirect
     * commands, we set it as this value, so we can map to the entity buffer
     * i.e. in shaders scene.entityData[SV_InstanceID] -> this entity
     *
     * Set with:
     * If !entityToIndex.contains(entity):
     * entityToIndex.insert(entity,entityToIndex.size())
     **/
    std::unordered_map<Entity, uint32_t> entityToIndex;
};

}