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

    void init(VkDevice device,
              uint32_t initialSets,
              std::span<PoolSizeRatio> poolRatios);
    void clear_pools();
    void destroy_pools();

    VkDescriptorSet allocate(VkDescriptorSetLayout layout,
                             const void* pNext = nullptr);

  private:
    VkDescriptorPool get_pool();
    VkDescriptorPool create_pool(uint32_t setCount,
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
    void add_binding(uint32_t binding, VkDescriptorType type);
    void clear();

    [[nodiscard]] VkDescriptorSetLayout build(
        VkDevice device,
        VkShaderStageFlags shaderStages,
        const void* pNext = nullptr,
        VkDescriptorSetLayoutCreateFlags flags = 0);

    // For a case where shader stages are different and set manually
    [[nodiscard]] VkDescriptorSetLayout build(
        VkDevice device,
        const void* pNext = nullptr,
        VkDescriptorSetLayoutCreateFlags flags = 0);

    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

class DescriptorWriter
{
  public:
    void write_image(int binding,
                    VkImageView image,
                    VkSampler sampler,
                    VkImageLayout layout,
                    VkDescriptorType type);
    void write_buffer(int binding,
                     VkBuffer buffer,
                     size_t size,
                     size_t offset,
                     VkDescriptorType type);

    void write(VkWriteDescriptorSet set);

    void clear();
    void update_set(VkDevice device, VkDescriptorSet set);

  private:
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;
};

struct TextureCache
{
    uint32_t add_texture(VkImageView imageView, VkSampler sampler);
    std::vector<VkDescriptorImageInfo> descriptors;
};

}
