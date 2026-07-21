#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {
class PipelineBuilder
{
  public:
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    VkVertexInputBindingDescription vertexBindingDescription;
    VkVertexInputAttributeDescription vertexAttributeDescription;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineLayout pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineRenderingCreateInfo renderInfo;
    VkFormat colorAttachmentformat;

    PipelineBuilder() { Clear(); }

    VkPipeline build_pipeline(VkDevice device);

    void set_vertex_input_none();
    void set_vertex_input_float_array();
    void set_shaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void set_input_topology(VkPrimitiveTopology topology);
    void set_polygon_mode(VkPolygonMode polygonMode);
    void set_cull_mode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void set_mutisampling_none();
    void disable_blending();
    void set_colour_attachment(VkFormat format);
    void set_depth_attachment(VkFormat format);
    void disable_depth_test();
    void enable_depth_test(bool depthWriteEnable, VkCompareOp op);

    void Clear();
};
}