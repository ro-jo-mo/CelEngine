#pragma once

#include "ecs/ResourceManager.h"

namespace Cel::Renderer {

class MaterialHandle;
class MeshHandle;

// After loading a gltf we create an asset handle
class AssetHandle
{};

// Comments:
// If an object is made up of a hierarchy of models, like a scene might be I
// can't create this tree in the ECS. Existing in the ECS would implicitly mean
// it exists in the world. Additionally it would be difficult to structure

// Workflow:
// Get asset handle from server
// Then you can add this asset to an entity through a method that adds all child
// nodes as distinct child entities in the ecs.
// At a later point I can optimise this, baking parts of the gltf that don't
// need to be their own entity

class AssetServer
{
  public:
    AssetServer(ResourceManager& resourceManager);
    AssetHandle LoadAsset(const char* path);
    void AddAssetToEntity(Entity entity,
                          Resource<World>& world,
                          AssetHandle asset);

  private:
};
}
