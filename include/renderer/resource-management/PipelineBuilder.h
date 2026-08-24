#pragma once

#include "renderer/VulkanTypes.h"

#include <map>
#include <spirv_reflect.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {

struct DescriptorSetLayoutData
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkDescriptorBindingFlags> flags;
    uint32_t layoutIndex;
};

/**
 * Most of the pipeline can default to the same set of assumptions about are
 * data
 */
class PipelineBuilder
{
  public:
    explicit PipelineBuilder(VkDevice device)
        : device(device)
    {
        initialise_defaults();
    }

    Pipeline build();

    /**
     * @brief Add a shader module to the pipeline
     * The stage, descriptors, push constants are derived through spirv reflect
     * @param path Path to the shader *.spv
     * @param stage Shader stage
     * @return this
     */
    PipelineBuilder& add_shader_module(const char* path,
                                       VkShaderStageFlagBits stage);

    PipelineBuilder& enable_depth_test(
        bool depthWriteEnable,
        VkCompareOp op = VK_COMPARE_OP_GREATER_OR_EQUAL);
    PipelineBuilder& disable_depth_test();
    PipelineBuilder& set_color_attachement(VkFormat format);
    PipelineBuilder& set_depth_attachment(VkFormat format);

  private:
    void generate_pipeline_layout();

    void initialise_defaults();

    void reflect_descriptor_data(const SpvReflectEntryPoint* entry,
                                 VkShaderStageFlagBits stage);

    void reflect_push_data(const SpvReflectShaderModule& spvModule,
                           const SpvReflectEntryPoint* entry,
                           VkShaderStageFlagBits stage);

    void reflect_vertex_data(const SpvReflectShaderModule& spvModule,
                             const SpvReflectEntryPoint* entry);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    struct LayoutBindingAndFlags
    {
        VkDescriptorSetLayoutBinding binding;
        VkDescriptorBindingFlags flags;
    };
    // descriptor (set,binding) -> vk data
    std::map<std::pair<uint32_t, uint32_t>, LayoutBindingAndFlags>
        descriptorSetLayouts;
    std::vector<VkDescriptorSetLayout> compiledDescriptors;

    std::vector<VkPushConstantRange> pushConstants;

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
    VkVertexInputBindingDescription bindingDescription;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineLayout pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineRenderingCreateInfo renderInfo;
    VkFormat colorAttachmentformat;

    VkDevice device;
};

}