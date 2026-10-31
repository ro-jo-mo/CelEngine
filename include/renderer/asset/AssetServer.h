#pragma once

#include "../../core/World.h"
#include "../Descriptors.h"
#include "../RenderTypes.h"
#include "../resource-management/MegaBuffer.h"
#include "AssetTypes.h"
#include "GltfLoader.h"
#include "ImageLoader.h"
#include "ProceduralLoader.h"
#include "common/Handle.h"
#include "ecs/Types.h"

namespace Cel::Renderer::RenderGraph {
class PassServer;
class Graph;
class PassBuilder;
struct RenderPass;
}
// Pre declarations
namespace Cel::Renderer {
class Camera;
namespace Asset {
class AssetServer;
}
struct DrawData;
void
cleanup_asset_server(Resource<Asset::AssetServer>& assetServer);
}

namespace Cel::Renderer::Passes {
struct PassFriend;
struct SceneData;

}

namespace Cel::Renderer::Asset {

// Comments:

// Workflow:
// Get asset handle from server
// Then you can add this asset to an entity through a method that adds all child
// nodes as distinct child entities in the ecs.
// At a later point I can optimise this, baking parts of the gltf that don't
// need to be their own entity

// A caching mechanism should be used on several levels
// Firstly we should not load the same gltf file twice
// Keep a map of file paths to asset handle
// The same material and mesh may be used in the same model several times
// For example wheels on a car

// Asset unloading:
// Once all references to an asset are deleted, i.e. no entities exist with this
// asset handle It should be added to a deletion queue. While in the deletion
// queue, it should still be accessible through the asset cache
// If loaded again, take out of the deletion queue
// Otherwise, whenever we determine more memory is required
// flush the deletion queue (until we have enough memory? or until empty queue?)

class AssetServer
{
  public:
    explicit AssetServer(VulkanResourceManager& manager,
                         Resource<SharedAssetData>& shared);

    Handle<SceneAsset> load_gltf(std::string file, GltfLabel::Label label);

    void add_scene_to_entity(Entity entity, Handle<SceneAsset> handle);

    Handle<ImageAsset> load_image(std::string file);

    ImageAsset& get_image(Handle<ImageAsset> handle);

    Handle<TextureAsset> load_texture(
        std::string file,
        SamplerSettings sampler = SamplerSettings::Linear);

    uint32_t get_texture(Handle<TextureAsset> handle);

    Handle<Primitive> create_cube(glm::vec3 size = glm::vec3(1.0f));

    Primitive& get_primitive(Handle<Primitive> handle);

    /**
     * Registers that this render pass accesses loaded assets.
     * I make possibly problematic assumptions about the pipeline stages
     *
     * @param pass
     */
    void declare_scene_access(RenderGraph::PassBuilder& pass);

    void flush_cpu(VulkanResourceManager& manager,
                   World& world,
                   SharedAssetData& shared,
                   RenderGraph::PassBuilder& pass);

    void flush_gpu(SharedAssetData& shared,
                   ParallelResource<RenderGraph::PassServer>& server);

    void cleanup();

  private:
    GltfLoader gltfLoader;
    ImageLoader imageLoader;
    ProceduralLoader proceduralLoader;

    // We guarantee that these handles are constant for the program runtime
    // Storing them here makes declare_scene_access more ergonomic
    Handle<AllocatedBuffer> vertexBufferHandle;
    Handle<AllocatedBuffer> indexBufferHandle;
    Handle<AllocatedBuffer> materialBufferHandle;
};

}
