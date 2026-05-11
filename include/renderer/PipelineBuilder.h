#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {
class PipelineBuilder
{
  public:
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineLayout pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineRenderingCreateInfo renderInfo;
    VkFormat colorAttachmentformat;

    PipelineBuilder() { Clear(); }

    VkPipeline BuildPipeline(VkDevice device);

    void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void SetInputTopology(VkPrimitiveTopology topology);
    void SetPolygonMode(VkPolygonMode polygonMode);
    void SetCullMode(VkCullModeFlags cullMode);
    void SetMutisamplingNone();
    void DisableBlending();
    void SetColourAttachment(VkFormat format);
    void SetDepthAttachment(VkFormat format);
    void DisableDepthTest();

    void Clear();
};
}