#include "renderer/passes/BasePasses.h"

#include "renderer/Camera.h"
#include "renderer/RenderComponents.h"
#include "renderer/SceneData.h"
#include "renderer/asset/AssetServer.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/render-graph/RenderGraph.h"
#include "renderer/resource-management/Queues.h"

using namespace Cel;

namespace {

inline const auto lightDataStagingBuffer =
    Cel::Renderer::Passes::HandleAllocator::allocate_buffer(
        "light_data_staging_buffer");

}

void
Cel::Renderer::Passes::PassFriend::register_create_scene_data_pass(
    Resource<RenderGraph::Graph>& graph)
{
    auto pass = RenderGraph::PassBuilder(Passes::createSceneDataPass)
                    .set_queue(Queues::transfer);

    pass // Scene data
        .create_buffer(Passes::sceneDataBuffer,
                       false,
                       sizeof(SceneData::sceneData),
                       VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY)
        .upload_buffer(sizeof(SceneData::sceneData),
                       Passes::sceneDataStagingBuffer,
                       Passes::sceneDataBuffer)
        // Entity data
        .create_buffer(Passes::entityDataBuffer,
                       false,
                       sizeof(PerEntityGpuData) * MAX_ENTITIES, // Scalar layout
                       VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT |
                           VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY)
        .upload_buffer(sizeof(PerEntityGpuData) * MAX_ENTITIES,
                       Passes::entityDataStagingBuffer,
                       Passes::entityDataBuffer)
        .create_buffer(Passes::lightDataBuffer,
                       true,
                       LIGHT_BUFFER_SIZE,
                       VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
                           VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY)
        .upload_buffer(
            LIGHT_BUFFER_SIZE, lightDataStagingBuffer, Passes::lightDataBuffer);

    graph->add_setup_pass(pass.build());
}

void
upload_light_data(
    VkCommandBuffer cmd,
    const Renderer::AllocatedBuffer& buffer,
    const Renderer::AllocatedBuffer& staging,
    Resource<Renderer::Passes::SceneData>& sceneData,
    Query<Get<Renderer::DirectionalLight>>& directionalLights,
    Query<Get<Renderer::PointLight, GlobalTransform>>& pointLights)
{
    sceneData->lightData = {
        .directionalLightCount =
            static_cast<uint32_t>(directionalLights.size()),
        .pointLightCount = static_cast<uint32_t>(pointLights.size()),
        .directionalLightOffset = 16,
        .pointLightOffset = static_cast<uint32_t>(
            16 + directionalLights.size() * sizeof(Renderer::DirectionalLight))
    };

    std::vector<Renderer::PointLight::Data> pointLightData;
    pointLightData.reserve(pointLights.size());

    for (auto [light, trans] : pointLights) {
        pointLightData.push_back(
            { .light = light, .position = trans.get_translation() });
    }

    std::vector<Renderer::DirectionalLight> directionalLightData;
    directionalLightData.reserve(directionalLights.size());

    for (auto [light] : directionalLights) {
        directionalLightData.push_back(light);
    }

    // Copy into light data
    size_t offset =
        directionalLightData.size() * sizeof(Renderer::DirectionalLight);

    memcpy(
        sceneData->lightData.data.data(), directionalLightData.data(), offset);
    
    memcpy(sceneData->lightData.data.data() + offset,
           pointLightData.data(),
           pointLightData.size() * sizeof(Renderer::PointLight::Data));
    ;

    Renderer::Utils::upload_to_buffer(
        cmd, &sceneData->lightData, LIGHT_BUFFER_SIZE, buffer, 0, staging);
}

void
Cel::Renderer::Passes::PassFriend::create_scene_data(
    Query<Get<Handle<Asset::Material>, GlobalTransform>>& entities,
    Query<Get<DirectionalLight>>& directionalLights,
    Query<Get<PointLight, GlobalTransform>>& pointLights,
    Query<Get<Camera, GlobalTransform>>& camera,
    Resource<SceneData>& sceneData,
    Resource<Asset::SharedAssetData>& shared,
    ParallelResource<RenderGraph::PassServer>& passServer)
{
    VkCommandBuffer cmd;
    {
        auto server = passServer.write();

        cmd = server->get_cmd_buffer(Passes::createSceneDataPass);
    }

    const auto& [cam, camTrans] = *camera.begin();

    auto access = passServer.partial_read();

    VkBufferDeviceAddressInfo vertInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = shared->vertexBuffer.buffer.buffer
    };
    VkBufferDeviceAddressInfo entityInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = access->get_resource(entityDataBuffer).buffer
    };
    VkBufferDeviceAddressInfo materialInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = shared->materialBuffer.buffer.buffer
    };

    const auto vertexAddress =
        vkGetBufferDeviceAddress(access->get_device(), &vertInfo);

    const auto materialAddress =
        vkGetBufferDeviceAddress(access->get_device(), &materialInfo);

    const auto entityAddress =
        vkGetBufferDeviceAddress(access->get_device(), &entityInfo);

    sceneData->sceneData = {
        .vertexBufferAddress = vertexAddress,
        .materialBufferAddress = materialAddress,
        .entityBufferAddress = entityAddress,
        .viewMatrix = cam.get_view_matrix(),
        .projectionMatrix = cam.get_projection_matrix(access->get_extent()),
        .viewProjMatrix = cam.get_projection_matrix(access->get_extent()) *
                          cam.get_view_matrix()
    };

    std::vector<PerEntityGpuData> entityData{ sceneData->entityToIndex.size() };

    for (auto [entity, index] : sceneData->entityToIndex) {
        // We guarantee that these entities will be here
        const auto& [mat, trans] = entities.get(entity);

        entityData[index] = { .transform = trans.transform,
                              .normalTransform = glm::transpose(
                                  glm::inverse(glm::mat3(trans.transform))),
                              .materialIndex = mat.index };
    }

    Utils::upload_to_buffer(
        cmd,
        &sceneData->sceneData,
        sizeof(sceneData->sceneData),
        access->get_resource(Passes::sceneDataBuffer),
        0,
        access->get_resource(Passes::sceneDataStagingBuffer));

    if (entityData.empty()) {
        return;
    }

    Utils::upload_to_buffer(
        cmd,
        entityData.data(),
        entityData.size() * sizeof(PerEntityGpuData),
        access->get_resource(Passes::entityDataBuffer),
        0,
        access->get_resource(Passes::entityDataStagingBuffer));
}

void
Cel::Renderer::Passes::PassFriend::register_asset_upload_pass(
    Resource<Asset::AssetServer>& server,
    Resource<VulkanResourceManager>& manager,
    Resource<Asset::SharedAssetData>& shared,
    Resource<World>& world,
    Resource<RenderGraph::Graph>& graph)
{
    auto pass =
        RenderGraph::PassBuilder(uploadAssetsPass).set_queue(Queues::transfer);

    server->flush_cpu(*manager, *world, *shared, pass);

    graph->add_setup_pass(pass.build());
}

void
Cel::Renderer::Passes::PassFriend::upload_assets(
    Resource<Asset::AssetServer>& assetServer,
    Resource<Asset::SharedAssetData>& shared,
    ParallelResource<RenderGraph::PassServer>& passServer)
{
    assetServer->flush_gpu(*shared, passServer);
}
