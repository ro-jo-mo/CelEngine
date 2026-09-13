#include "renderer/SceneData.h"

uint32_t
Cel::Renderer::Passes::SceneData::get_entity_index(const Entity entity)
{
    return entityToIndex.try_emplace(entity, entityToIndex.size())
        .first->second;
}
