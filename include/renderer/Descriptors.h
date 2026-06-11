#pragma once
#include "VulkanTypes.h"

#include <deque>
#include <span>

namespace Cel::Renderer {
class DescriptorAllocator
{
  public:
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    void Init(VkDevice device,
              uint32_t initialSets,
              std::span<PoolSizeRatio> poolRatios);
    void ClearPools(VkDevice device);
    void DestroyPools(VkDevice device);

    VkDescriptorSet Allocate(VkDevice device,
                             VkDescriptorSetLayout layout,
                             const void* pNext = nullptr);

  private:
    VkDescriptorPool GetPool(VkDevice device);
    VkDescriptorPool CreatePool(VkDevice device,
                                uint32_t setCount,
                                std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    uint32_t setsPerPool;
};

class DescriptorLayoutBuilder
{
  public:
    void AddBinding(uint32_t binding, VkDescriptorType type);
    void Clear();
    VkDescriptorSetLayout Build(VkDevice device,
                                VkShaderStageFlags shaderStages,
                                const void* pNext = nullptr,
                                VkDescriptorSetLayoutCreateFlags flags = 0);

  private:
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class DescriptorWriter
{
  public:
    void WriteImage(int binding,
                    VkImageView image,
                    VkSampler sampler,
                    VkImageLayout layout,
                    VkDescriptorType type);
    void WriteBuffer(int binding,
                     VkBuffer buffer,
                     size_t size,
                     size_t offset,
                     VkDescriptorType type);

    void Clear();
    void UpdateSet(VkDevice device, VkDescriptorSet set);

  private:
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;
};

class TextureCache
{

  public:
    uint32_t AddTexture(VkImageView imageView, VkSampler sampler);

  private:
    std::vector<VkDescriptorImageInfo> descriptors;
};
};
