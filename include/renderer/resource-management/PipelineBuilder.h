#pragma once

#include "renderer/RenderTypes.h"

#include <deque>
#include <map>
#include <spirv_reflect.h>
#include <string>
#include <string_view>
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
    explicit PipelineBuilder(VkDevice device);

    Pipeline build();

    /**
     * @brief Add a single stage from a compiled shader to the pipeline
     * The descriptors, push constants and vertex input are derived through
     * spirv reflect
     * @param name Shader name relative to the compiled shaders folder
     * @param stage The stage whose entry point should be used
     * @return this
     */
    PipelineBuilder& add_shader_module(std::string_view name,
                                       VkShaderStageFlagBits stage);

    /**
     * @brief Add both the vertex and fragment stages from a compiled shader.
     * @param name Shader name relative to the compiled shaders folder
     * @return this
     */
    PipelineBuilder& add_graphics_shader(std::string_view name);

    PipelineBuilder& enable_depth_test(
        bool depthWriteEnable,
        VkCompareOp op = VK_COMPARE_OP_GREATER_OR_EQUAL);
    PipelineBuilder& disable_depth_test();
    PipelineBuilder& set_color_attachment(VkFormat format);
    PipelineBuilder& set_depth_attachment(VkFormat format);
    PipelineBuilder& set_winding_order(VkFrontFace frontFace);
    PipelineBuilder& set_cull_mode(VkCullModeFlags cullMode);

    static void initialise_default_descriptor(VkDevice device);

    inline static VkDescriptorSetLayout defaultSetLayout = VK_NULL_HANDLE;
    inline static VkPipelineLayout defaultPipelineLayout = VK_NULL_HANDLE;

    // We support only the minimum guaranteed by vulkan 1.3
    static constexpr VkPushConstantRange defaultPushRange{
        .stageFlags = VK_SHADER_STAGE_ALL,
        .offset = 0,
        .size = 128
    };

  private:
    struct LoadedShader
    {
        SpvReflectShaderModule reflection;
        VkShaderModule module;
    };

    /**
     * @brief Read a compiled shader from disk, reflect it and create its
     * VkShaderModule. The builder owns the module and destroys it in build().
     * The caller must destroy the reflection with spvReflectDestroyShaderModule
     */
    LoadedShader load_shader(std::string_view name);

    /**
     * @brief Add the entry point for stage to the pipeline and reflect its
     * descriptors, push constants and vertex input
     */
    void add_shader_module(const SpvReflectShaderModule* spvModule,
                           VkShaderModule shaderModule,
                           VkShaderStageFlagBits stage);

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
    // Unique modules, as several stages may share one
    std::vector<VkShaderModule> shaderModules;
    // Owns the strings that shaderStages[i].pName points at. A deque, so
    // growing it doesn't move existing strings
    std::deque<std::string> entryPointNames;

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