#include "renderer/Descriptors.h"
#include "renderer/VulkanHelpers.h"

#include <cassert>
#include <ranges>
#include <vulkan/vulkan_core.h>

void
Cel::Renderer::DescriptorAllocator::init(
    VkDevice vkdevice,

    const uint32_t initialSets,
    const std::span<PoolSizeRatio> poolRatios)
{
    device = vkdevice;

    ratios.clear();
    for (auto r : poolRatios) {
        ratios.push_back(r);
    }

    VkDescriptorPool newPool = create_pool(initialSets, poolRatios);

    setsPerPool = initialSets * 1.5;

    readyPools.push_back(newPool);
}

void
Cel::Renderer::DescriptorAllocator::clear_pools()
{
    for (auto p : readyPools) {
        vkResetDescriptorPool(device, p, 0);
    }
    for (auto p : fullPools) {
        vkResetDescriptorPool(device, p, 0);
        readyPools.push_back(p);
    }
    fullPools.clear();
}

void
Cel::Renderer::DescriptorAllocator::destroy_pools()
{
    for (auto p : readyPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    readyPools.clear();
    for (auto p : fullPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    fullPools.clear();
}

VkDescriptorSet
Cel::Renderer::DescriptorAllocator::allocate(VkDescriptorSetLayout layout,
                                             const void* pNext)
{
    // get or create a pool to allocate from
    VkDescriptorPool poolToUse = get_pool();

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.pNext = pNext;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = poolToUse;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

    // allocation failed. Try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY ||
        result == VK_ERROR_FRAGMENTED_POOL) {

        fullPools.push_back(poolToUse);
        poolToUse = get_pool();
        allocInfo.descriptorPool = poolToUse;

        vk_check(vkAllocateDescriptorSets(device, &allocInfo, &ds));
    }

    readyPools.push_back(poolToUse);
    return ds;
}

VkDescriptorPool
Cel::Renderer::DescriptorAllocator::get_pool()
{
    VkDescriptorPool newPool;
    if (readyPools.size() != 0) {
        newPool = readyPools.back();
        readyPools.pop_back();
    } else {
        // need to create a new pool
        newPool = create_pool(setsPerPool, ratios);

        setsPerPool = setsPerPool * 1.5;
        if (setsPerPool > 4092) {
            setsPerPool = 4092;
        }
    }

    return newPool;
}

VkDescriptorPool
Cel::Renderer::DescriptorAllocator::create_pool(
    const uint32_t setCount,
    const std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios) {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = ratio.type,
            .descriptorCount = uint32_t(ratio.ratio * setCount) });
    }

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = setCount;
    pool_info.poolSizeCount = (uint32_t)poolSizes.size();
    pool_info.pPoolSizes = poolSizes.data();

    VkDescriptorPool newPool;
    vkCreateDescriptorPool(device, &pool_info, nullptr, &newPool);
    return newPool;
}

void
Cel::Renderer::DescriptorLayoutBuilder::add_binding(const uint32_t binding,
                                                   VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newBind{};
    newBind.binding = binding;
    newBind.descriptorCount = 1;
    newBind.descriptorType = type;

    bindings.push_back(newBind);
}

void
Cel::Renderer::DescriptorLayoutBuilder::clear()
{
    bindings.clear();
}

VkDescriptorSetLayout
Cel::Renderer::DescriptorLayoutBuilder::build(
    VkDevice device,
    VkShaderStageFlags shaderStages,
    const void* pNext,
    VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& binding : bindings) {
        binding.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    };
    info.pNext = pNext;

    info.pBindings = bindings.data();
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.flags = flags;

    VkDescriptorSetLayout set;
    vk_check(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
}

VkDescriptorSetLayout
Cel::Renderer::DescriptorLayoutBuilder::build(
    VkDevice device,
    const void* pNext,
    VkDescriptorSetLayoutCreateFlags flags)
{
    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    };
    info.pNext = pNext;

    info.pBindings = bindings.data();
    info.bindingCount = static_cast<uint32_t>(bindings.size());
    info.flags = flags;

    VkDescriptorSetLayout set;
    vk_check(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
}

void
Cel::Renderer::DescriptorWriter::write_image(const int binding,
                                            VkImageView image,
                                            VkSampler sampler,
                                            VkImageLayout layout,
                                            VkDescriptorType type)
{
    const VkDescriptorImageInfo& info =
        imageInfos.emplace_back(VkDescriptorImageInfo{
            .sampler = sampler, .imageView = image, .imageLayout = layout });

    VkWriteDescriptorSet write = { .sType =
                                       VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    write.dstBinding = binding;
    write.dstSet =
        VK_NULL_HANDLE; // left empty for now until we need to write it
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &info;

    writes.push_back(write);
}

void
Cel::Renderer::DescriptorWriter::write_buffer(const int binding,
                                             VkBuffer buffer,
                                             const size_t size,
                                             const size_t offset,
                                             VkDescriptorType type)
{
    VkDescriptorBufferInfo& info =
        bufferInfos.emplace_back(VkDescriptorBufferInfo{
            .buffer = buffer, .offset = offset, .range = size });

    VkWriteDescriptorSet write = { .sType =
                                       VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };

    write.dstBinding = binding;
    write.dstSet =
        VK_NULL_HANDLE; // left empty for now until we need to write it
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pBufferInfo = &info;

    writes.push_back(write);
}

void
Cel::Renderer::DescriptorWriter::write(VkWriteDescriptorSet set)
{
    writes.push_back(set);
}

void
Cel::Renderer::DescriptorWriter::clear()
{
    imageInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

void
Cel::Renderer::DescriptorWriter::update_set(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : writes) {
        write.dstSet = set;
    }
    for (auto& w : writes) {
        assert(w.sType == VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
        assert(w.dstSet != VK_NULL_HANDLE);
        assert(w.descriptorCount > 0);

        if (w.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
            w.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
            w.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
            assert(w.pImageInfo != nullptr);

        if (w.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
            w.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
            assert(w.pBufferInfo != nullptr);
    }
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(writes.size()),
                           writes.data(),
                           0,
                           nullptr);
}

uint32_t
Cel::Renderer::TextureCache::add_texture(VkImageView imageView,
                                        VkSampler sampler)
{
    for (const auto& [i, descriptor] :
         std::ranges::views::enumerate(descriptors)) {
        if (descriptor.imageView == imageView &&
            descriptor.sampler == sampler) {
            return i;
        }
    }

    const uint32_t i = descriptors.size();

    descriptors.push_back(VkDescriptorImageInfo{
        .sampler = sampler,
        .imageView = imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

    return i;
}
