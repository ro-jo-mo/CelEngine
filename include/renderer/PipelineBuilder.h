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

    void Clear();
};
}