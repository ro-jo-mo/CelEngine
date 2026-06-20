#pragma once

#include <deque>
#include <span>
#include <vector>
#include <vulkan/vulkan.h>

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
    void ClearPools();
    void DestroyPools();

    VkDescriptorSet Allocate(VkDescriptorSetLayout layout,
                             const void* pNext = nullptr);

  private:
    VkDescriptorPool GetPool();
    VkDescriptorPool CreatePool(uint32_t setCount,
                                std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    uint32_t setsPerPool;
    VkDevice device;
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

    void Write(VkWriteDescriptorSet set);

    void Clear();
    void UpdateSet(VkDevice device, VkDescriptorSet set);

  private:
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;
};

struct TextureCache
{
    uint32_t AddTexture(VkImageView imageView, VkSampler sampler);
    std::vector<VkDescriptorImageInfo> descriptors;
};

}
