#include "renderer/passes/BasePasses.h"

#include "renderer/AssetServer.h"
#include "renderer/Camera.h"
#include "renderer/Queues.h"
#include "renderer/SceneData.h"
#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/render-graph/RenderGraph.h"

void
Cel::Renderer::Passes::PassFriend::register_present_pass(
    Resource<RenderGraph::Graph>& graph)
{
    auto pass = RenderGraph::PassBuilder(Passes::presentPass)
                    .read_image(Passes::drawImage,
                                VK_PIPELINE_STAGE_2_BLIT_BIT,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                VK_ACCESS_2_TRANSFER_READ_BIT)
                    .set_queue(Queues::graphics);

    graph->set_present_pass(pass.build());
}

void
Cel::Renderer::Passes::PassFriend::register_create_scene_data_pass(
    Resource<RenderGraph::Graph>& graph)
{
    auto pass = RenderGraph::PassBuilder(Passes::createSceneDataPass)
                    .set_queue(Queues::transfer);

    pass // Scene data
        .create_buffer(Passes::sceneDataStagingBuffer,
                       true,
                       sizeof(SceneData::data),
                       VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
                       VMA_MEMORY_USAGE_CPU_TO_GPU)
        .create_buffer(Passes::sceneDataBuffer,
                       false,
                       sizeof(SceneData::data),
                       VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY)
        .upload_buffer(Passes::sceneDataStagingBuffer, Passes::sceneDataBuffer)
        // Entity data
        .create_buffer(Passes::entityDataStagingBuffer,
                       true,
                       sizeof(PerEntityGpuData) * MAX_ENTITIES,
                       VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
                       VMA_MEMORY_USAGE_CPU_TO_GPU)
        .create_buffer(Passes::entityDataBuffer,
                       false,
                       sizeof(PerEntityGpuData) * MAX_ENTITIES, // Scalar layout
                       VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT |
                           VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
                       VMA_MEMORY_USAGE_GPU_ONLY)
        .upload_buffer(Passes::entityDataStagingBuffer,
                       Passes::entityDataBuffer);

    graph->add_setup_pass(pass.build());
}

void
Cel::Renderer::Passes::PassFriend::create_and_bind_scene_data(
    Query<With<Handle<Assets::Material>, GlobalTransform>>& entities,
    Query<With<Camera, GlobalTransform>>& camera,
    Resource<SceneData>& sceneData,
    Resource<Assets::AssetServer>& assetServer,
    ParallelResource<RenderGraph::PassServer>& passServer)
{
    const auto& [cam, camTrans] = *camera.begin();

    auto access = passServer.partial_read();

    VkBufferDeviceAddressInfo vertInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = assetServer->vertexBuffer.buffer.buffer
    };
    VkBufferDeviceAddressInfo indiceInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = assetServer->indiceBuffer.buffer.buffer
    };
    VkBufferDeviceAddressInfo materialInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = assetServer->materialBuffer.buffer.buffer
    };

    const auto vertexAddress =
        vkGetBufferDeviceAddress(assetServer->device, &vertInfo);
    const auto indiceAddress =
        vkGetBufferDeviceAddress(assetServer->device, &indiceInfo);
    const auto materialAddress =
        vkGetBufferDeviceAddress(assetServer->device, &materialInfo);

    sceneData->data = { .vertexBufferAddress = vertexAddress,
                        .materialBufferAddress = indiceAddress,
                        .perEntityBufferAddress = materialAddress,
                        .viewMatrix = cam.get_view_matrix(),
                        .projectionMatrix =
                            cam.get_projection_matrix(access->get_extent()),
                        .viewProjMatrix =
                            cam.get_projection_matrix(access->get_extent()) *
                            cam.get_view_matrix() };

    std::vector<PerEntityGpuData> entityData{ sceneData->entityToIndex.size() };

    for (auto [entity, index] : sceneData->entityToIndex) {
        // We guarantee that these entities will be here
        const auto& [mat, trans] = entities.get(entity);

        entityData[index] = { .transform = trans.transform,
                              .normalTransform = glm::transpose(
                                  glm::inverse(glm::mat3(trans.transform))),
                              .materialIndex = mat.index };
    }

    VkDescriptorSetLayout baseDescLayout;
    {
        DescriptorLayoutBuilder builder;

        builder.add_binding(
            0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL);

        VkDescriptorSetLayoutBinding textures{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = MAX_VARIABLE_DESCRIPTOR_ARRAY,
            .stageFlags = VK_SHADER_STAGE_ALL,
            .pImmutableSamplers = nullptr
        };

        builder.add_binding(textures,
                            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);

        baseDescLayout = builder.build(access->get_device());
    }

    VkPipelineLayout baseLayout;
    {
        VkPipelineLayoutCreateInfo baseLayoutCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = 1,
            .pSetLayouts = &baseDescLayout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr
        };

        vkCreatePipelineLayout(
            access->get_device(), &baseLayoutCreateInfo, nullptr, &baseLayout);
    }

    DescriptorWriter writer;
    {
        writer.write_buffer(
            0,
            access->get_resource(Passes::sceneDataBuffer).buffer,
            sizeof(sceneData->data),
            0,
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

        if (!assetServer->textureCache.descriptors.empty()) {
            VkWriteDescriptorSet arraySet;
            arraySet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            arraySet.descriptorCount =
                assetServer->textureCache.descriptors.size();
            arraySet.dstArrayElement = 0;
            arraySet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            arraySet.dstBinding = 1;
            arraySet.pImageInfo = assetServer->textureCache.descriptors.data();
            arraySet.pNext = nullptr;

            writer.write(arraySet);
        }
    }

    VkDescriptorSet baseDescSet;
    VkCommandBuffer cmd;
    {
        auto server = passServer.write();

        cmd = server->get_cmd_buffer(Passes::createSceneDataPass);

        baseDescSet =
            server->get_descriptor_allocator().allocate(baseDescLayout);

        writer.update_set(access->get_device(), baseDescSet);
    }

    Utils::upload_to_buffer(
        cmd,
        &sceneData->data,
        sizeof(sceneData->data),
        access->get_resource(Passes::sceneDataBuffer),
        0,
        access->get_resource(Passes::sceneDataStagingBuffer));

    Utils::upload_to_buffer(
        cmd,
        entityData.data(),
        entityData.size(),
        access->get_resource(Passes::entityDataBuffer),
        0,
        access->get_resource(Passes::entityDataStagingBuffer));

    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            baseLayout,
                            0,
                            1,
                            &baseDescSet,
                            0,
                            nullptr);

    vkCmdBindIndexBuffer(
        cmd, assetServer->indiceBuffer.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
}

void
Cel::Renderer::Passes::PassFriend::register_asset_upload_pass(
    Resource<Assets::AssetServer>& server,
    Resource<VulkanResourceManager>& manager,
    Resource<RenderGraph::Graph>& graph)
{
    auto pass =
        RenderGraph::PassBuilder(uploadAssetsPass).set_queue(Queues::transfer);

    server->register_pass(pass, manager);

    graph->add_setup_pass(pass.build());
}

void
Cel::Renderer::Passes::PassFriend::upload_assets(
    Resource<Assets::AssetServer>& assetServer,
    ParallelResource<RenderGraph::PassServer>& passServer)
{
    assetServer->flush(passServer);
}
