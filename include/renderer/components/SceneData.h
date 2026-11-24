#pragma once

#include "../resource-management/MegaBuffer.h"
#include "Camera.h"
#include "ecs/Types.h"

namespace Cel::Renderer::Passes {

// A resource containing data about the scene
struct SceneData
{
    // Struct that we'll push to the gpu
    struct
    {
        VkDeviceAddress vertexBufferAddress;
        VkDeviceAddress materialBufferAddress;
        VkDeviceAddress entityBufferAddress;
        VkDeviceAddress lightBufferAddress;
        Camera::Data camera;
    } sceneData;

    struct
    {
        uint32_t directionalLightCount;
        uint32_t pointLightCount;
        uint32_t directionalLightOffset;
        uint32_t pointLightOffset;

        std::array<char, LIGHT_BUFFER_SIZE - 16> data;
    } lightData;

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

    uint32_t get_entity_index(Entity entity);
};

}