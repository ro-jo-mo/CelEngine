#include "renderer/Descriptors.h"
#include "renderer/VulkanHelpers.h"

#include <cassert>
#include <ranges>
#include <vulkan/vulkan_core.h>

void
Cel::Renderer::DescriptorAllocator::Init(
    VkDevice vkdevice,

    const uint32_t initialSets,
    const std::span<PoolSizeRatio> poolRatios)
{
    device = vkdevice;

    ratios.clear();
    for (auto r : poolRatios) {
        ratios.push_back(r);
    }

    VkDescriptorPool newPool = CreatePool(initialSets, poolRatios);

    setsPerPool = initialSets * 1.5;

    readyPools.push_back(newPool);
}

void
Cel::Renderer::DescriptorAllocator::ClearPools()
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
Cel::Renderer::DescriptorAllocator::DestroyPools()
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
Cel::Renderer::DescriptorAllocator::Allocate(VkDescriptorSetLayout layout,
                                             const void* pNext)
{
    // get or create a pool to allocate from
    VkDescriptorPool poolToUse = GetPool();

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
        poolToUse = GetPool();
        allocInfo.descriptorPool = poolToUse;

        VkCheck(vkAllocateDescriptorSets(device, &allocInfo, &ds));
    }

    readyPools.push_back(poolToUse);
    return ds;
}

VkDescriptorPool
Cel::Renderer::DescriptorAllocator::GetPool()
{
    VkDescriptorPool newPool;
    if (readyPools.size() != 0) {
        newPool = readyPools.back();
        readyPools.pop_back();
    } else {
        // need to create a new pool
        newPool = CreatePool(setsPerPool, ratios);

        setsPerPool = setsPerPool * 1.5;
        if (setsPerPool > 4092) {
            setsPerPool = 4092;
        }
    }

    return newPool;
}

VkDescriptorPool
Cel::Renderer::DescriptorAllocator::CreatePool(
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
Cel::Renderer::DescriptorLayoutBuilder::AddBinding(const uint32_t binding,
                                                   VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newBind{};
    newBind.binding = binding;
    newBind.descriptorCount = 1;
    newBind.descriptorType = type;

    bindings.push_back(newBind);
}

void
Cel::Renderer::DescriptorLayoutBuilder::Clear()
{
    bindings.clear();
}

VkDescriptorSetLayout
Cel::Renderer::DescriptorLayoutBuilder::Build(
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
    VkCheck(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

    return set;
}

void
Cel::Renderer::DescriptorWriter::WriteImage(const int binding,
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
Cel::Renderer::DescriptorWriter::WriteBuffer(const int binding,
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
Cel::Renderer::DescriptorWriter::Write(VkWriteDescriptorSet set)
{
    writes.push_back(set);
}

void
Cel::Renderer::DescriptorWriter::Clear()
{
    imageInfos.clear();
    writes.clear();
    bufferInfos.clear();
}

void
Cel::Renderer::DescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : writes) {
        write.dstSet = set;
    }
    
    vkUpdateDescriptorSets(device,
                           static_cast<uint32_t>(writes.size()),
                           writes.data(),
                           0,
                           nullptr);
}

uint32_t
Cel::Renderer::TextureCache::AddTexture(VkImageView imageView,
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
