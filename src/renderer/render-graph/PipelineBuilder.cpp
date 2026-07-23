#include "renderer/render-graph/PipelineBuilder.h"

#include "core/Error.h"
#include "renderer/VulkanHelpers.h"
#include "renderer/VulkanUtils.h"

#include <fstream>
#include <ranges>
#include <spirv_reflect.h>
#include <vulkan/vulkan_core.h>

using namespace Cel::Renderer;

auto assert_spv = [](SpvReflectResult result) {
    if (result != SPV_REFLECT_RESULT_SUCCESS) {
        Cel::throw_error("error during shader reflection: {}",
                         static_cast<uint32_t>(result));
    }
};

Pipeline
PipelineBuilder_::build()
{
    generate_pipeline_layout();

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // setup dummy color blending. We arent using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
    };
    // connect the renderInfo to the pNext extension mechanism
    pipelineInfo.pNext = &renderInfo;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = pipelineLayout;

    VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT,
                               VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamicInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO
    };
    dynamicInfo.pDynamicStates = &state[0];
    dynamicInfo.dynamicStateCount = 2;

    pipelineInfo.pDynamicState = &dynamicInfo;

    VkPipeline newPipeline;

    if (vkCreateGraphicsPipelines(
            device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) ==
        VK_SUCCESS) {
        fmt::println("failed to create pipeline");
        return { newPipeline, pipelineLayout };
    }

    // if failure return null
    return { VK_NULL_HANDLE, VK_NULL_HANDLE };
}

PipelineBuilder_&
PipelineBuilder_::enable_depth_test(bool depthWriteEnable, VkCompareOp op)
{
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = depthWriteEnable;
    depthStencil.depthCompareOp = op;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};
    depthStencil.minDepthBounds = 0.f;
    depthStencil.maxDepthBounds = 1.f;

    return *this;
}

PipelineBuilder_&
PipelineBuilder_::disable_depth_test()
{
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};
    depthStencil.minDepthBounds = 0.f;
    depthStencil.maxDepthBounds = 1.f;

    return *this;
}

PipelineBuilder_&
PipelineBuilder_::set_color_attachement(VkFormat format)
{
    colorAttachmentformat = format;

    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachmentFormats = &colorAttachmentformat;

    return *this;
}

PipelineBuilder_&
PipelineBuilder_::set_depth_attachment(VkFormat format)
{
    renderInfo.depthAttachmentFormat = format;

    return *this;
}

/**
 * The default pipeline *should* be good enough to represent 90% of graphics
 * pipelines accurately without changes
 */
void
PipelineBuilder_::initialise_defaults()
{
    vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

    inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    // Disable colour blending
    colorBlendAttachment = { .blendEnable = VK_FALSE,
                             .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                               VK_COLOR_COMPONENT_G_BIT |
                                               VK_COLOR_COMPONENT_B_BIT |
                                               VK_COLOR_COMPONENT_A_BIT };
    // Disable multisampling
    multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    pipelineLayout = {};

    // Enable depth testing
    depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };
    enable_depth_test(true);

    // Set generic colour / depth attachments
    set_color_attachement(VK_FORMAT_R16G16B16A16_SFLOAT);
    set_depth_attachment(VK_FORMAT_D32_SFLOAT);

    renderInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
}

void
PipelineBuilder_::generate_pipeline_layout()
{
    VkPipelineLayoutCreateInfo createInfo =
        Initialisers::pipeline_layout_create_info();

    // Set push constants (simple)
    createInfo.pushConstantRangeCount = pushConstants.size();
    createInfo.pPushConstantRanges = pushConstants.data();

    // Set descriptor layouts (need to finalise descriptor data)
    uint32_t currentSet = 0;

    // The key of the map is as so: (set,binding slot)
    if (descriptorSetLayouts.begin() != descriptorSetLayouts.end()) {
        currentSet = descriptorSetLayouts.begin()->first.first;
    }

    DescriptorLayoutBuilder builder{};

    std::vector<VkDescriptorSetLayout> compiledDescriptorLayouts;

    // Simply add the descriptor bindings to the builder
    // If its a new set, build and push to final descriptor list
    for (const auto& [setAndBinding, layoutBinding] : descriptorSetLayouts) {
        const auto set = setAndBinding.first;

        if (currentSet != set) {
            compiledDescriptorLayouts.push_back(builder.build(device));

            builder.clear();

            currentSet = set;
        }

        builder.bindings.push_back(layoutBinding);
    }

    // Don't forget to compile the final descriptor
    if (builder.bindings.size() > 0) {
        compiledDescriptorLayouts.push_back(builder.build(device));
    }

    createInfo.setLayoutCount = compiledDescriptorLayouts.size();
    createInfo.pSetLayouts = compiledDescriptorLayouts.data();

    vkCreatePipelineLayout(device, &createInfo, nullptr, &pipelineLayout);
}

DescriptorSetLayoutData
get_descriptor_data_from_spv(const SpvReflectDescriptorSet& layout,
                             VkShaderStageFlagBits stage)
{
    DescriptorSetLayoutData data{};
    data.layoutIndex = layout.set;
    data.createInfo.bindingCount = layout.binding_count;
    data.bindings.reserve(layout.binding_count);
    data.flags.resize(layout.binding_count);

    for (size_t i = 0; i < layout.binding_count; i++) {
        VkDescriptorSetLayoutBinding bindingInfo{};

        auto binding = layout.bindings[i];

        bindingInfo.binding = binding->binding;
        bindingInfo.descriptorCount = binding->count;
        bindingInfo.descriptorType =
            static_cast<VkDescriptorType>(binding->descriptor_type);
        bindingInfo.stageFlags = stage;

        // if run time descriptor array, we must set flags
        if (binding->array.dims_count > 0 && binding->array.dims[0] == 0) {
            data.flags[i] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                            VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
        }

        data.bindings.push_back(bindingInfo);
    }

    return data;
}

uint32_t
format_to_byte_size(const VkFormat format)
{
    switch (format) {
        case VK_FORMAT_R32G32B32_SFLOAT:
            return 12;
        default:
            Cel::throw_error("unimplemented vk format {}",
                             static_cast<uint32_t>(format));
            return 0;
    }
}

PipelineBuilder_&
PipelineBuilder_::add_shader_module(const char* path,
                                    VkShaderStageFlagBits stage)
{
    // open the file. With cursor at the end
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw_error("unable to open shader file {}", std::move(path));
    }

    // The cursors is at the end of the file, so it can be used to get the
    // file size in bytes
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()),
              static_cast<std::streamsize>(fileSize));
    file.close();

    // codeSize is measured in bytes
    auto codeSize = buffer.size() * sizeof(uint32_t);

    SpvReflectShaderModule spvModule{};
    assert_spv(
        spvReflectCreateShaderModule(codeSize, buffer.data(), &spvModule));

    VkShaderModule shaderModule;
    if (!Utils::load_shader(buffer.data(), codeSize, device, &shaderModule)) {
        throw_error("Error loading shader {}", std::move(path));
    }

    // Gather only the data from this shader stage
    SpvReflectEntryPoint* entry = nullptr;
    for (size_t i = 0; i < spvModule.entry_point_count; i++) {
        if (spvModule.entry_points[i].shader_stage ==
            static_cast<SpvReflectShaderStageFlagBits>(stage)) {
            entry = &spvModule.entry_points[i];
        }
    }

    if (entry == nullptr) {
        throw_error(
            "couldn't find entry point in shader. \nEntry {} \nShader {}",
            static_cast<uint32_t>(stage),
            std::move(path));
        return *this;
    }

    VkPipelineShaderStageCreateInfo shaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = shaderModule,
        .pName = entry->name
    };

    shaderStages.push_back(shaderStageInfo);

    // Grab descriptor layout
    reflect_descriptor_data(entry, stage);

    // Grab push  constants
    reflect_push_data(spvModule, entry, stage);

    // If this is the vertex stage, reflect the vertex data
    if (stage == VK_SHADER_STAGE_VERTEX_BIT) {
        reflect_vertex_data(spvModule, entry);
    }

    return *this;
}
void
PipelineBuilder_::reflect_descriptor_data(const SpvReflectEntryPoint* entry,
                                          const VkShaderStageFlagBits stage)
{
    for (size_t i = 0; i < entry->descriptor_set_count; i++) {
        DescriptorSetLayoutData data =
            get_descriptor_data_from_spv(entry->descriptor_sets[i], stage);
        VkDescriptorSetLayoutCreateInfo x;
        uint32_t set = entry->descriptor_sets[i].set;
        // Merge the bindings with existing data
        for (auto& binding : data.bindings) {
            std::pair location = { set, binding.binding };

            if (descriptorSetLayouts.contains(location)) {
                // Assign this stage to the binding as well as its existing
                // stage
                auto& val = descriptorSetLayouts.at(location);
                val.stageFlags |= stage;
                // basic correctness check
                assert(val.descriptorType == binding.descriptorType &&
                       val.descriptorCount == binding.descriptorCount);
            } else {
                descriptorSetLayouts.emplace(location, binding);
            }
        }
    }
}

void
PipelineBuilder_::reflect_push_data(const SpvReflectShaderModule& spvModule,
                                    const SpvReflectEntryPoint* entry,
                                    VkShaderStageFlagBits stage)
{
    uint32_t pushCount;

    assert_spv(spvReflectEnumerateEntryPointPushConstantBlocks(
        &spvModule, entry->name, &pushCount, nullptr));
    std::vector<SpvReflectBlockVariable*> blockVars{ pushCount };
    assert_spv(spvReflectEnumerateEntryPointPushConstantBlocks(
        &spvModule, entry->name, &pushCount, blockVars.data()));

    pushConstants.reserve(pushConstants.size() + pushCount);
    for (size_t i = 0; i < pushCount; i++) {
        SpvReflectBlockVariable& blockVar = *blockVars[i];

        bool isUnique = true;
        for (auto& pushConstant : pushConstants) {
            if (pushConstant.offset == blockVar.offset &&
                pushConstant.size == blockVar.size) {
                pushConstant.stageFlags |= stage;
                isUnique = false;
                break;
            }
        }

        if (isUnique) {
            pushConstants.emplace_back(stage, blockVar.offset, blockVar.size);
        }
    }
}

void
PipelineBuilder_::reflect_vertex_data(const SpvReflectShaderModule& spvModule,
                                      const SpvReflectEntryPoint* entry)
{
    uint32_t inputCount;
    assert_spv(spvReflectEnumerateEntryPointInputVariables(
        &spvModule, entry->name, &inputCount, nullptr));

    std::vector<SpvReflectInterfaceVariable*> interfaceVars{ inputCount };
    assert_spv(spvReflectEnumerateEntryPointInputVariables(
        &spvModule, entry->name, &inputCount, interfaceVars.data()));

    // We have fixed assumptions about the vertex input
    // Assume vertex buffer is only bound to slot 0, only one buffer
    // The vertex input is exactly as stated in the shader, no compression

    // stride and offset must be computed later
    bindingDescription = {
        .binding = 0,
        .stride = 0,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    attributeDescriptions.reserve(inputCount);

    for (size_t i = 0; i < inputCount; i++) {
        auto& interfaceVar = *interfaceVars[i];

        // skip built ins
        if (interfaceVar.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
            continue;
        }

        VkVertexInputAttributeDescription attribute{
            .location = interfaceVar.location,
            .binding = bindingDescription.binding,
            .format = static_cast<VkFormat>(interfaceVar.format)
        };

        attributeDescriptions.push_back(attribute);
    }

    // With the bindless architecture most shaders shouldn't need a vertex input
    if (attributeDescriptions.size() == 0) {
        return;
    }

    // Sort the attribute data so we can calculate stride + offset
    std::ranges::sort(attributeDescriptions,
                      [](auto& a, auto& b) { return a.location < b.location; });

    for (auto& attribute : attributeDescriptions) {
        const uint32_t formatSize = format_to_byte_size(attribute.format);
        attribute.offset = bindingDescription.stride;
        bindingDescription.stride += formatSize;
    }

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;

    vertexInputInfo.vertexAttributeDescriptionCount =
        attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
}
