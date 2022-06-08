/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#include "node_context_descriptor_set_manager_vk.h"

#include <algorithm>
#include <cstdint>
#include <vulkan/vulkan.h>

#include <base/math/mathf.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
#include "nodecontext/node_context_descriptor_set_manager.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/gpu_image_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
NodeContextDescriptorSetManagerVk::NodeContextDescriptorSetManagerVk(Device& device)
    : NodeContextDescriptorSetManager(), device_ { device },
      bufferingCount_(
          Math::min(LowLevelContextDescriptorPoolVk::MAX_BUFFERING_COUNT, device_.GetCommandBufferingCount()))
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (device_.GetCommandBufferingCount() > LowLevelContextDescriptorPoolVk::MAX_BUFFERING_COUNT) {
        PLUGIN_LOG_ONCE_W("device_command_buffering_count_desc_set_vk_buffering",
            "RENDER_VALIDATION: device command buffering count (%u) is larger than supported vulkan descriptor set "
            "buffering count (%u)",
            device_.GetCommandBufferingCount(), LowLevelContextDescriptorPoolVk::MAX_BUFFERING_COUNT);
    }
#endif
}

NodeContextDescriptorSetManagerVk::~NodeContextDescriptorSetManagerVk()
{
    DestroyPool(descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_STATIC]);
    DestroyPool(descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME]);
    for (auto& ref : pendingDeallocations_) {
        DestroyPool(ref.descriptorPool);
    }
}

void NodeContextDescriptorSetManagerVk::DestroyPool(LowLevelContextDescriptorPoolVk& descriptorPool)
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;

    for (auto& ref : descriptorPool.descriptorSets) {
        for (uint32_t bufferingIdx = 0; bufferingIdx < bufferingCount_; ++bufferingIdx) {
            auto& bufferingSetRef = ref.bufferingSet[bufferingIdx];
            if (bufferingSetRef.descriptorSetLayout) {
                vkDestroyDescriptorSetLayout(device,     // device
                    bufferingSetRef.descriptorSetLayout, // descriptorSetLayout
                    nullptr);                            // pAllocator
                bufferingSetRef.descriptorSetLayout = VK_NULL_HANDLE;
            }
        }
        if (ref.additionalPlatformSet.descriptorSetLayout) {
            vkDestroyDescriptorSetLayout(device,               // device
                ref.additionalPlatformSet.descriptorSetLayout, // descriptorSetLayout
                nullptr);                                      // pAllocator
            ref.additionalPlatformSet.descriptorSetLayout = VK_NULL_HANDLE;
        }
    }
    descriptorPool.descriptorSets.clear();
    if (descriptorPool.descriptorPool) {
        vkDestroyDescriptorPool(device,    // device
            descriptorPool.descriptorPool, // descriptorPool
            nullptr);                      // pAllocator
        descriptorPool.descriptorPool = VK_NULL_HANDLE;
    }
    if (descriptorPool.additionalPlatformDescriptorPool) {
        vkDestroyDescriptorPool(device,                      // device
            descriptorPool.additionalPlatformDescriptorPool, // descriptorPool
            nullptr);                                        // pAllocator
        descriptorPool.additionalPlatformDescriptorPool = VK_NULL_HANDLE;
    }
}

void NodeContextDescriptorSetManagerVk::ResetAndReserve(const DescriptorCounts& descriptorCounts)
{
    NodeContextDescriptorSetManager::ResetAndReserve(descriptorCounts);
    if (maxSets_ > 0) {
        auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
        // usually there are less descriptor sets than max sets count
        // due to maxsets count has been calculated for single descriptors
        // (one descriptor set has multiple descriptors)
        const uint32_t reserveCount = maxSets_ / 2u; // questimate for possible max vector size;

        constexpr VkDescriptorPoolCreateFlags descriptorPoolCreateFlags { 0 };
        if (descriptorPool.descriptorPool) { // push for dealloation vec
            PendingDeallocations pd;
            pd.descriptorPool.descriptorPool = move(descriptorPool.descriptorPool);
            pd.descriptorPool.descriptorSets = move(descriptorPool.descriptorSets);
            pd.frameIndex = device_.GetFrameCount();
            pendingDeallocations_.emplace_back(move(pd));

            descriptorPool.descriptorSets.clear();
            descriptorPool.descriptorPool = VK_NULL_HANDLE;
        }

        descriptorPoolSizes_.clear();
        descriptorPoolSizes_.reserve(descriptorCounts.counts.size()); // max count reserve
        for (const auto& ref : descriptorCounts.counts) {
            if (ref.count > 0) {
                descriptorPoolSizes_.emplace_back(
                    VkDescriptorPoolSize { (VkDescriptorType)ref.type, ref.count * bufferingCount_ });
            }
        }

        if (!descriptorPoolSizes_.empty()) {
            const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
                VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, // sType
                nullptr,                                       // pNext
                descriptorPoolCreateFlags,                     // flags
                maxSets_ * bufferingCount_,                    // maxSets
                (uint32_t)descriptorPoolSizes_.size(),         // poolSizeCount
                descriptorPoolSizes_.data(),                   // pPoolSizes
            };

            const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
            VALIDATE_VK_RESULT(vkCreateDescriptorPool(device, // device
                &descriptorPoolCreateInfo,                    // pCreateInfo
                nullptr,                                      // pAllocator
                &descriptorPool.descriptorPool));             // pDescriptorPool

            descriptorPool.descriptorSets.reserve(reserveCount);
        }
    }
}

void NodeContextDescriptorSetManagerVk::BeginFrame()
{
    NodeContextDescriptorSetManager::BeginFrame();

    oneFrameDescriptorNeed_ = {};
    auto& oneFrameDescriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];
    if (oneFrameDescriptorPool.descriptorPool) {
        const uint32_t descriptorSetCount = static_cast<uint32_t>(oneFrameDescriptorPool.descriptorSets.size());
        PendingDeallocations pd;
        pd.descriptorPool.descriptorPool = move(oneFrameDescriptorPool.descriptorPool);
        pd.descriptorPool.descriptorSets = move(oneFrameDescriptorPool.descriptorSets);
        pd.frameIndex = device_.GetFrameCount();
        pendingDeallocations_.emplace_back(move(pd));

        oneFrameDescriptorPool.descriptorSets.reserve(descriptorSetCount);
        oneFrameDescriptorPool.descriptorPool = VK_NULL_HANDLE;
    }
    oneFrameDescriptorPool.descriptorSets.clear();

    // we need to check through platform special format desriptor sets/pool
    auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
    if (descriptorPool.additionalPlatformDescriptorPool) {
        PendingDeallocations pd;
        pd.descriptorPool.additionalPlatformDescriptorPool = move(descriptorPool.additionalPlatformDescriptorPool);
        // no buffering set
        pd.frameIndex = device_.GetFrameCount();
        pendingDeallocations_.emplace_back(move(pd));
        descriptorPool.additionalPlatformDescriptorPool = VK_NULL_HANDLE;
        // immediate desctruction of descriptor set layouts
        const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
        for (auto& descriptorSetRef : descriptorPool.descriptorSets) {
            if (descriptorSetRef.additionalPlatformSet.descriptorSetLayout) {
                vkDestroyDescriptorSetLayout(device,                            // device
                    descriptorSetRef.additionalPlatformSet.descriptorSetLayout, // descriptorSetLayout
                    nullptr);                                                   // pAllocator
                descriptorSetRef.additionalPlatformSet.descriptorSetLayout = VK_NULL_HANDLE;
            }
        }
        auto& cpuDescriptorSet = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
        for (auto& descriptorSetRef : cpuDescriptorSet) {
            descriptorSetRef.hasPlatformConversionBindings = false;
        }
    }

    // clear aged descriptor pools
    if (!pendingDeallocations_.empty()) {
        // this is normally empty or only has single item
        const auto minAge = device_.GetCommandBufferingCount() + 1;
        const auto ageLimit = (device_.GetFrameCount() < minAge) ? 0 : (device_.GetFrameCount() - minAge);

        auto oldRes = std::partition(pendingDeallocations_.begin(), pendingDeallocations_.end(),
            [ageLimit](auto const& pd) { return pd.frameIndex >= ageLimit; });

        std::for_each(oldRes, pendingDeallocations_.end(), [this](auto& res) { DestroyPool(res.descriptorPool); });
        pendingDeallocations_.erase(oldRes, pendingDeallocations_.end());
    }

#if (RENDER_VALIDATION_ENABLED == 1)
    oneFrameDescSetGeneration_ = (oneFrameDescSetGeneration_ + 1) % MAX_ONE_FRAME_GENERATION_IDX;
#endif
}

void NodeContextDescriptorSetManagerVk::BeginBackendFrame()
{
    auto CreateDescriptorPool = [](const VkDevice device, const uint32_t descriptorSetCount,
                                    VkDescriptorPool& descriptorPool,
                                    vector<VkDescriptorPoolSize>& descriptorPoolSizes) {
        constexpr VkDescriptorPoolCreateFlags descriptorPoolCreateFlags { 0 };
        const VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
            VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, // sType
            nullptr,                                       // pNext
            descriptorPoolCreateFlags,                     // flags
            descriptorSetCount,                            // maxSets
            (uint32_t)descriptorPoolSizes.size(),          // poolSizeCount
            descriptorPoolSizes.data(),                    // pPoolSizes
        };

        VALIDATE_VK_RESULT(vkCreateDescriptorPool(device, // device
            &descriptorPoolCreateInfo,                    // pCreateInfo
            nullptr,                                      // pAllocator
            &descriptorPool));                            // pDescriptorPool
    };

    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    // reserve descriptors for descriptors sets that need platform special formats for one frame
    if (hasPlatformConversionBindings_) {
        const auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
        uint32_t descriptorSetCount = 0u;
        uint8_t descriptorCounts[OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE] { 0 };
        for (const auto& cpuDescriptorSetRef : cpuDescriptorSets) {
            if (cpuDescriptorSetRef.hasPlatformConversionBindings) {
                descriptorSetCount++;
                for (const auto& bindingRef : cpuDescriptorSetRef.bindings) {
                    uint32_t descriptorCount = bindingRef.binding.descriptorCount;
                    const uint32_t descTypeIndex = static_cast<uint32_t>(bindingRef.binding.descriptorType);
                    if (descTypeIndex < OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE) {
                        if ((bindingRef.binding.descriptorType ==
                                DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                            RenderHandleUtil::IsPlatformConversionResource(
                                cpuDescriptorSetRef.images[bindingRef.resourceIndex].resource.handle)) {
                            // expecting planar formats and making sure that there is enough descriptors
                            constexpr uint32_t descriptorCountMultiplier = 3u;
                            descriptorCount *= descriptorCountMultiplier;
                        }
                        descriptorCounts[descTypeIndex] += static_cast<uint8_t>(descriptorCount);
                    }
                }
            }
        }
        if (descriptorSetCount > 0) {
            auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
            PLUGIN_ASSERT(descriptorPool.additionalPlatformDescriptorPool == VK_NULL_HANDLE);
            descriptorPoolSizes_.clear();
            descriptorPoolSizes_.reserve(OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE);
            // no buffering, only descriptors for one frame
            for (uint32_t idx = 0; idx < OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE; ++idx) {
                const uint8_t count = descriptorCounts[idx];
                if (count > 0) {
                    descriptorPoolSizes_.emplace_back(VkDescriptorPoolSize { (VkDescriptorType)idx, count });
                }
            }
            if (!descriptorPoolSizes_.empty()) {
                CreateDescriptorPool(
                    device, descriptorSetCount, descriptorPool.additionalPlatformDescriptorPool, descriptorPoolSizes_);
            }
        }
    }
    // create one frame descriptor pool
    {
        auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];
        auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];

        PLUGIN_ASSERT(descriptorPool.descriptorPool == VK_NULL_HANDLE);
        const uint32_t descriptorSetCount = static_cast<uint32_t>(cpuDescriptorSets.size());
        if (descriptorSetCount > 0) {
            descriptorPoolSizes_.clear();
            descriptorPoolSizes_.reserve(OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE);
            for (uint32_t idx = 0; idx < OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE; ++idx) {
                const uint8_t count = oneFrameDescriptorNeed_.descriptorCount[idx];
                if (count > 0) {
                    descriptorPoolSizes_.emplace_back(VkDescriptorPoolSize { (VkDescriptorType)idx, count });
                }
            }

            if (!descriptorPoolSizes_.empty()) {
                CreateDescriptorPool(device, descriptorSetCount, descriptorPool.descriptorPool, descriptorPoolSizes_);
            }
        }
    }
}

RenderHandle NodeContextDescriptorSetManagerVk::CreateDescriptorSet(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    RenderHandle clientHandle;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
    auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
    PLUGIN_ASSERT_MSG(cpuDescriptorSets.size() < maxSets_, "no more descriptor sets available");
    if (cpuDescriptorSets.size() < maxSets_) {
        uint32_t dynamicOffsetCount = 0;
        CpuDescriptorSet newSet;
        LowLevelContextDescriptorPoolVk::DescriptorSetData descSetData;

        newSet.bindings.reserve(descriptorSetLayoutBindings.size());
        descSetData.descriptorCounts.writeDescriptorCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
        for (const auto& refBinding : descriptorSetLayoutBindings) {
            // NOTE: sort from 0 to n
            newSet.bindings.push_back({ refBinding, {} });
            if (IsDynamicDescriptor(refBinding.descriptorType)) {
                dynamicOffsetCount++;
            }

            const uint32_t descriptorCount = refBinding.descriptorCount;
            if (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
                descSetData.descriptorCounts.samplerCount += descriptorCount;
            } else if (((refBinding.descriptorType >= CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                           (refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE)) ||
                       (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
                descSetData.descriptorCounts.imageCount += refBinding.descriptorCount;
            } else if (((refBinding.descriptorType >= CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) &&
                           (refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) ||
                       (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE)) {
                descSetData.descriptorCounts.bufferCount += refBinding.descriptorCount;
            } else {
                PLUGIN_ASSERT_MSG(false, "descriptor type not found");
            }
        }
        newSet.buffers.resize(descSetData.descriptorCounts.bufferCount);
        newSet.images.resize(descSetData.descriptorCounts.imageCount);
        newSet.samplers.resize(descSetData.descriptorCounts.samplerCount);

        const uint32_t arrayIndex = (uint32_t)cpuDescriptorSets.size();
        cpuDescriptorSets.emplace_back(move(newSet));

        auto& currCpuDescriptorSet = cpuDescriptorSets[arrayIndex];
        currCpuDescriptorSet.dynamicOffsetDescriptors.resize(dynamicOffsetCount);

        // allocate storage from vector to gpu descriptor sets
        // don't create the actual gpu descriptor sets yet
        descriptorPool.descriptorSets.emplace_back(descSetData);

        // NOTE: can be used directly to index
        clientHandle = RenderHandleUtil::CreateHandle(RenderHandleType::DESCRIPTOR_SET, arrayIndex, 0);
    }

    return clientHandle;
}

RenderHandle NodeContextDescriptorSetManagerVk::CreateOneFrameDescriptorSet(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    RenderHandle clientHandle;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];
    auto& descriptorPool = descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];
    uint32_t dynamicOffsetCount = 0;
    CpuDescriptorSet newSet;
    LowLevelContextDescriptorPoolVk::DescriptorSetData descSetData;

    newSet.bindings.reserve(descriptorSetLayoutBindings.size());
    descSetData.descriptorCounts.writeDescriptorCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
    for (const auto& refBinding : descriptorSetLayoutBindings) {
        // NOTE: sort from 0 to n
        newSet.bindings.push_back({ refBinding, {} });
        if (IsDynamicDescriptor(refBinding.descriptorType)) {
            dynamicOffsetCount++;
        }

        if (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
            descSetData.descriptorCounts.samplerCount += refBinding.descriptorCount;
        } else if (((refBinding.descriptorType >= CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                       (refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE)) ||
                   (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
            descSetData.descriptorCounts.imageCount += refBinding.descriptorCount;
        } else if (((refBinding.descriptorType >= CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) &&
                       (refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) ||
                   (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE)) {
            descSetData.descriptorCounts.bufferCount += refBinding.descriptorCount;
        } else {
            PLUGIN_LOG_E("descriptor type not found");
        }

        newSet.buffers.resize(descSetData.descriptorCounts.bufferCount);
        newSet.images.resize(descSetData.descriptorCounts.imageCount);
        newSet.samplers.resize(descSetData.descriptorCounts.samplerCount);

        if (static_cast<uint32_t>(refBinding.descriptorType) < OneFrameDescriptorNeed::DESCRIPTOR_ARRAY_SIZE) {
            oneFrameDescriptorNeed_.descriptorCount[refBinding.descriptorType] +=
                static_cast<uint8_t>(refBinding.descriptorCount);
        }
    }

    const uint32_t arrayIndex = static_cast<uint32_t>(cpuDescriptorSets.size());
    cpuDescriptorSets.emplace_back(move(newSet));

    auto& currCpuDescriptorSet = cpuDescriptorSets[arrayIndex];
    currCpuDescriptorSet.dynamicOffsetDescriptors.resize(dynamicOffsetCount);

    // allocate storage from vector to gpu descriptor sets
    // don't create the actual gpu descriptor sets yet
    descriptorPool.descriptorSets.emplace_back(descSetData);

    // NOTE: can be used directly to index
    clientHandle = RenderHandleUtil::CreateHandle(
        RenderHandleType::DESCRIPTOR_SET, arrayIndex, oneFrameDescSetGeneration_, ONE_FRAME_DESC_SET_BIT);

    return clientHandle;
}

void NodeContextDescriptorSetManagerVk::CreateGpuDescriptorSet(const uint32_t bufferCount,
    const RenderHandle clientHandle, const CpuDescriptorSet& cpuDescriptorSet,
    LowLevelContextDescriptorPoolVk& descriptorPool)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (cpuDescriptorSet.bindings.size() > PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
        PLUGIN_LOG_W("RENDER_VALIDATION: descriptor set binding count exceeds (max:%u, current:%u)",
            PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT,
            static_cast<uint32_t>(cpuDescriptorSet.bindings.size()));
    }
#endif
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(clientHandle);
    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT];
    const uint32_t bindingCount = Math::min(static_cast<uint32_t>(cpuDescriptorSet.bindings.size()),
        PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT);
    const bool hasPlatformBindings = cpuDescriptorSet.hasPlatformConversionBindings;
    bool hasImmutableSamplers = false;
    uint16_t immutableSamplerBitmask = 0;
    const auto& gpuResourceMgr = static_cast<const GpuResourceManager&>(device_.GetGpuResourceManager());
    // NOTE: if we cannot provide explicit flags that custom immutable sampler with conversion is needed
    // we should first loop through the bindings and check
    // normal hw buffers do not need any rebindings or immutable samplers
    for (uint32_t idx = 0; idx < bindingCount; ++idx) {
        const DescriptorSetLayoutBindingResource& cpuBinding = cpuDescriptorSet.bindings[idx];
        const VkDescriptorType descriptorType = (VkDescriptorType)cpuBinding.binding.descriptorType;
        const VkShaderStageFlags stageFlags = (VkShaderStageFlags)cpuBinding.binding.shaderStageFlags;
        const uint32_t bindingIdx = cpuBinding.binding.binding;
        const VkSampler* immutableSampler = nullptr;
        if (hasPlatformBindings && (descriptorType == VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) {
            const RenderHandle handle = cpuDescriptorSet.images[cpuBinding.resourceIndex].resource.handle;
            if (RenderHandleUtil::IsPlatformConversionResource(handle)) {
                if (const auto* gpuImage = static_cast<GpuImageVk*>(gpuResourceMgr.GetImage(handle)); gpuImage) {
                    const GpuImage::AdditionalFlags additionalFlags = gpuImage->GetAdditionalFlags();
                    immutableSampler = &(gpuImage->GetPlaformDataConversion().sampler);
                    if ((additionalFlags & GpuImage::AdditionalFlagBits::ADDITIONAL_PLATFORM_CONVERSION_BIT) &&
                        immutableSampler) {
                        hasImmutableSamplers = true;
                        immutableSamplerBitmask |= (1 << bindingIdx);
                    }
                }
            }
#if (RENDER_VALIDATION_ENABLED == 1)
            if (!immutableSampler) {
                PLUGIN_LOG_W("RENDER_VALIDATION: immutable sampler for platform conversion resource not found");
            }
#endif
        }
        descriptorSetLayoutBindings[idx] = {
            bindingIdx,                         // binding
            descriptorType,                     // descriptorType
            cpuBinding.binding.descriptorCount, // descriptorCount
            stageFlags,                         // stageFlags
            immutableSampler,                   // pImmutableSamplers
        };
    }

    constexpr VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateFlags { 0 };
    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, // sType
        nullptr,                                             // pNext
        descriptorSetLayoutCreateFlags,                      // flags
        bindingCount,                                        // bindingCount
        descriptorSetLayoutBindings,                         // pBindings
    };

    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    for (uint32_t idx = 0; idx < bufferCount; ++idx) {
        LowLevelDescriptorSetVk newDescriptorSet;
        newDescriptorSet.flags |=
            (hasImmutableSamplers) ? LowLevelDescriptorSetVk::DESCRIPTOR_SET_LAYOUT_IMMUTABLE_SAMPLER_BIT : 0u;
        newDescriptorSet.immutableSamplerBitmask = immutableSamplerBitmask;

        VALIDATE_VK_RESULT(vkCreateDescriptorSetLayout(device, // device
            &descriptorSetLayoutCreateInfo,                    // pCreateInfo
            nullptr,                                           // pAllocator
            &newDescriptorSet.descriptorSetLayout));           // pSetLayout

        // for immutable set we use created additional descriptor pool (currently only used with ycbcr)
        const VkDescriptorPool descriptorPoolVk = (immutableSamplerBitmask != 0)
                                                      ? descriptorPool.additionalPlatformDescriptorPool
                                                      : descriptorPool.descriptorPool;
        PLUGIN_ASSERT(descriptorPoolVk);
        const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // sType
            nullptr,                                        // pNext
            descriptorPoolVk,                               // descriptorPool
            1u,                                             // descriptorSetCount
            &newDescriptorSet.descriptorSetLayout,          // pSetLayouts
        };

        VALIDATE_VK_RESULT(vkAllocateDescriptorSets(device, // device
            &descriptorSetAllocateInfo,                     // pAllocateInfo
            &newDescriptorSet.descriptorSet));              // pDescriptorSets

        if (immutableSamplerBitmask != 0) {
            descriptorPool.descriptorSets[arrayIndex].additionalPlatformSet = newDescriptorSet;
        } else {
            PLUGIN_ASSERT(descriptorPool.descriptorSets[arrayIndex].bufferingSet[idx].descriptorSet == VK_NULL_HANDLE);
            descriptorPool.descriptorSets[arrayIndex].bufferingSet[idx] = newDescriptorSet;
        }
        // NOTE: descriptor sets could be tagged with debug name
        // might be a bit overkill to do it always
#if (RENDER_VALIDATION_ENABLED == 1)
        if (newDescriptorSet.descriptorSet == VK_NULL_HANDLE) {
            PLUGIN_LOG_E("RENDER_VALIDATION: gpu descriptor set creation failed, ds node: %s, ds binding count: %u",
                debugName_.c_str(), bindingCount);
        }
#endif
    }
}

const LowLevelDescriptorCountsVk& NodeContextDescriptorSetManagerVk::GetLowLevelDescriptorCounts(
    const RenderHandle handle)
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                            : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    const auto& descriptorPool = descriptorPool_[descSetIdx];
    if (arrayIndex < (uint32_t)descriptorPool.descriptorSets.size()) {
        return descriptorPool.descriptorSets[arrayIndex].descriptorCounts;
    } else {
        PLUGIN_LOG_E("invalid handle in descriptor set management");
        return defaultLowLevelDescriptorSetMemoryStoreVk_;
    }
}

const LowLevelDescriptorSetVk* NodeContextDescriptorSetManagerVk::GetDescriptorSet(const RenderHandle handle) const
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                            : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    const auto& cpuDescriptorSets = cpuDescriptorSets_[descSetIdx];
    const auto& descriptorPool = descriptorPool_[descSetIdx];
    const LowLevelDescriptorSetVk* set = nullptr;
    if (arrayIndex < (uint32_t)cpuDescriptorSets.size()) {
        if (arrayIndex < descriptorPool.descriptorSets.size()) {
            // additional set is only used there are platform buffer bindings and additional set created
            const bool useAdditionalSet =
                (cpuDescriptorSets[arrayIndex].hasPlatformConversionBindings &&
                    descriptorPool.descriptorSets[arrayIndex].additionalPlatformSet.descriptorSet);
            if (useAdditionalSet) {
                PLUGIN_ASSERT_MSG(descriptorPool.descriptorSets[arrayIndex].additionalPlatformSet.descriptorSet,
                    "descriptor set not updated");
                set = &descriptorPool.descriptorSets[arrayIndex].additionalPlatformSet;
            } else {
                const uint32_t bufferingIndex = cpuDescriptorSets[arrayIndex].currentGpuBufferingIndex;
                PLUGIN_ASSERT_MSG(descriptorPool.descriptorSets[arrayIndex].bufferingSet[bufferingIndex].descriptorSet,
                    "descriptor set not updated");
                set = &descriptorPool.descriptorSets[arrayIndex].bufferingSet[bufferingIndex];
            }
        }

#if (RENDER_VALIDATION_ENABLED == 1)
        if (set) {
            if (set->descriptorSet == VK_NULL_HANDLE) {
                PLUGIN_LOG_E("RENDER_VALIDATION: descriptor set has not been updated prior to binding");
                PLUGIN_LOG_E(
                    "RENDER_VALIDATION: gpu descriptor set created ? %u, descriptor set node: %s, buffer count: %u, "
                    "image count: %u, sampler count: %u",
                    (uint32_t)cpuDescriptorSets[arrayIndex].gpuDescriptorSetCreated, debugName_.c_str(),
                    (uint32_t)descriptorPool.descriptorSets[arrayIndex].descriptorCounts.bufferCount,
                    (uint32_t)descriptorPool.descriptorSets[arrayIndex].descriptorCounts.imageCount,
                    (uint32_t)descriptorPool.descriptorSets[arrayIndex].descriptorCounts.samplerCount);
            }
        }
#endif
    }

    return set;
}

void NodeContextDescriptorSetManagerVk::UpdateDescriptorSetGpuHandle(const RenderHandle handle)
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                            : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    auto& cpuDescriptorSets = cpuDescriptorSets_[descSetIdx];
    auto& descriptorPool = descriptorPool_[descSetIdx];
    const uint32_t bufferingCount = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? 1u : bufferingCount_;
    if (arrayIndex < (uint32_t)cpuDescriptorSets.size()) {
        CpuDescriptorSet& refCpuSet = cpuDescriptorSets[arrayIndex];

        // with platform buffer bindings descriptor set creation needs to be checked
        if (refCpuSet.hasPlatformConversionBindings) {
            // no buffering
            CreateGpuDescriptorSet(1u, handle, refCpuSet, descriptorPool);
        } else if (!refCpuSet.gpuDescriptorSetCreated) { // deferred creation
            CreateGpuDescriptorSet(bufferingCount, handle, refCpuSet, descriptorPool);
            refCpuSet.gpuDescriptorSetCreated = true;
        }

        if (refCpuSet.isDirty) {
            refCpuSet.isDirty = false;
            // advance to next gpu descriptor set
            if (oneFrameDescBit != ONE_FRAME_DESC_SET_BIT) {
                refCpuSet.currentGpuBufferingIndex = (refCpuSet.currentGpuBufferingIndex + 1) % bufferingCount_;
            }
        }
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("invalid handle in descriptor set management");
#endif
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) {
        const uint32_t generationIndex = RenderHandleUtil::GetGenerationIndexPart(handle);
        if (generationIndex != oneFrameDescSetGeneration_) {
            PLUGIN_LOG_E(
                "RENDER_VALIDATION: invalid one frame descriptor set handle generation. One frame descriptor sets "
                "can only be used once.");
        }
    }
#endif
}
RENDER_END_NAMESPACE()
