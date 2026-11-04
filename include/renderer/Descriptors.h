#pragma once

#include <array>
#include <deque>
#include <optional>
#include <span>
#include <unordered_map>
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

    DescriptorAllocator() = default;

    DescriptorAllocator(VkDevice device,
                        uint32_t initialSets,
                        std::span<PoolSizeRatio> poolRatios);

    void clear_pools();
    void destroy_pools();

    VkDescriptorSet allocate(VkDescriptorSetLayout layout, const void* pNext);

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
    void add_binding(uint32_t binding,
                     VkDescriptorType type,
                     VkShaderStageFlags stages,
                     VkDescriptorBindingFlags flagBits = 0);
    void add_binding(VkDescriptorSetLayoutBinding binding,
                     VkDescriptorBindingFlags flagBits = 0);
    void clear();
    [[nodiscard]] bool empty() const;

    [[nodiscard]] VkDescriptorSetLayout build(VkDevice device) const;

  private:
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkDescriptorBindingFlags> flags;
};

class DescriptorWriter
{
  public:
    void write_image(int binding,
                     VkImageView image,
                     VkSampler sampler,
                     VkImageLayout layout,
                     VkDescriptorType type);
    void write_buffer(uint32_t binding,
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

struct SamplerSettings
{
    VkFilter min = VK_FILTER_LINEAR;
    VkFilter mag = VK_FILTER_LINEAR;

    VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkSamplerAddressMode addressU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    // 0 disables anisotropic filtering. Non-zero requires the
    // samplerAnisotropy device feature
    float maxAnisotropy = 0.f;

    // Set for depth comparison samplers (shadow maps)
    std::optional<VkCompareOp> compare;

    VkBorderColor border = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

    bool operator==(const SamplerSettings&) const = default;

    // Sensible default for textures
    const static SamplerSettings Linear;

    // Sensible default for pixel art
    const static SamplerSettings Pixel;
};

inline constexpr SamplerSettings SamplerSettings::Linear{};
inline constexpr SamplerSettings SamplerSettings::Pixel{
    .min = VK_FILTER_NEAREST,
    .mag = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
};

struct SamplerSettingsHash
{
    size_t operator()(const SamplerSettings& s) const noexcept;
};

/**
 * Owns all samplers bound to the global sampler array.
 * Identical settings share a single VkSampler.
 */
class SamplerCache
{
  public:
    // Index of SamplerSettings::Linear, always present
    static constexpr uint32_t defaultSampler = 0;

    explicit SamplerCache(VkDevice device);

    // Returns the index into the sampler array, creating the sampler if needed
    uint32_t add_sampler(const SamplerSettings& settings);

    void cleanup();

    std::vector<VkDescriptorImageInfo> infos{};

  private:
    std::unordered_map<SamplerSettings, uint32_t, SamplerSettingsHash> lookup;

    VkDevice device;
};

/**
 * Images bound to the global sampled image array.
 * Samplers are bound separately, see SamplerCache.
 */
class TextureCache
{
  public:
    uint32_t add_texture(VkImageView view);

    std::vector<VkDescriptorImageInfo> infos;

  private:
    std::unordered_map<VkImageView, uint32_t> lookup;
};

}
