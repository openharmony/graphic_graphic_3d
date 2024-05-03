/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VULKAN_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_VK_H
#define VULKAN_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_handle_util.h"
#include "nodecontext/node_context_descriptor_set_manager.h"

RENDER_BEGIN_NAMESPACE()
struct RenderPassBeginInfo;
class GpuResourceManager;

struct LowLevelDescriptorSetVk {
    VkDescriptorSet descriptorSet { VK_NULL_HANDLE };
    // NOTE: descriptorSetLayout could be only one for buffering descriptor sets
    VkDescriptorSetLayout descriptorSetLayout { VK_NULL_HANDLE };

    enum DescriptorSetLayoutFlagBits : uint32_t {
        // immutable samplers bound in to the set (atm used only for ycbcr conversions which might need new psos)
        DESCRIPTOR_SET_LAYOUT_IMMUTABLE_SAMPLER_BIT = 0x00000001,
    };
    uint32_t flags { 0u };
    // has a bit set for the ones in 16 bindings that have immutable sampler
    uint16_t immutableSamplerBitmask { 0u };
};

// storage counts
struct LowLevelDescriptorCountsVk {
    uint32_t writeDescriptorCount { 0u };
    uint32_t bufferCount { 0u };
    uint32_t imageCount { 0u };
    uint32_t samplerCount { 0u };
    uint32_t accelCount { 0u };
};

struct LowLevelContextDescriptorPoolVk {
    // buffering count of 3 (max) is used often with vulkan triple buffering
    // gets the real used buffering count from the device
    static constexpr uint32_t MAX_BUFFERING_COUNT { 3u };

    VkDescriptorPool descriptorPool { VK_NULL_HANDLE };
    // additional descriptor pool for one frame descriptor sets with platform buffer bindings
    // reset and reserved every frame if needed, does not live through frames
    VkDescriptorPool additionalPlatformDescriptorPool { VK_NULL_HANDLE };

    struct DescriptorSetData {
        LowLevelDescriptorCountsVk descriptorCounts;
        // indexed with frame buffering index
        LowLevelDescriptorSetVk bufferingSet[MAX_BUFFERING_COUNT];

        // additional platform set used in case there are
        // e.g. ycbcr hwbufferw with immutable samplers and sampler conversion
        // one might change between normal combined_image_sampler every frame
        // these do not override the bufferingSet
        LowLevelDescriptorSetVk additionalPlatformSet;
    };
    BASE_NS::vector<DescriptorSetData> descriptorSets;
};

struct LowLevelContextDescriptorWriteDataVk {
    BASE_NS::vector<VkWriteDescriptorSet> writeDescriptorSets;
    BASE_NS::vector<VkDescriptorBufferInfo> descriptorBufferInfos;
    BASE_NS::vector<VkDescriptorImageInfo> descriptorImageInfos;
    BASE_NS::vector<VkDescriptorImageInfo> descriptorSamplerInfos;
#if (RENDER_VULKAN_RT_ENABLED == 1)
    BASE_NS::vector<VkWriteDescriptorSetAccelerationStructureKHR> descriptorAccelInfos;
#endif

    uint32_t writeBindingCount { 0U };
    uint32_t bufferBindingCount { 0U };
    uint32_t imageBindingCount { 0U };
    uint32_t samplerBindingCount { 0U };

    void Clear()
    {
        writeBindingCount = 0U;
        bufferBindingCount = 0U;
        imageBindingCount = 0U;
        samplerBindingCount = 0U;

        writeDescriptorSets.clear();
        descriptorBufferInfos.clear();
        descriptorImageInfos.clear();
        descriptorSamplerInfos.clear();
#if (RENDER_VULKAN_RT_ENABLED == 1)
        descriptorAccelInfos.clear();
#endif
    }
};

class NodeContextDescriptorSetManagerVk final : public NodeContextDescriptorSetManager {
public:
    explicit NodeContextDescriptorSetManagerVk(Device& device);
    ~NodeContextDescriptorSetManagerVk();

    void ResetAndReserve(const DescriptorCounts& descriptorCounts) override;
    void BeginFrame() override;
    // Call when starting processing this node context. Creates one frame descriptor pool.
    void BeginBackendFrame();

    RenderHandle CreateDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;
    RenderHandle CreateOneFrameDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;

    const LowLevelDescriptorCountsVk& GetLowLevelDescriptorCounts(const RenderHandle handle);
    const LowLevelDescriptorSetVk* GetDescriptorSet(const RenderHandle handle) const;

    LowLevelContextDescriptorWriteDataVk& GetLowLevelDescriptorWriteData();

    // deferred gpu descriptor set creation happens here
    void UpdateDescriptorSetGpuHandle(const RenderHandle handle) override;
    void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) override;

private:
    Device& device_;

    void CreateGpuDescriptorSet(const uint32_t bufferCount, const RenderHandle clientHandle,
        const CpuDescriptorSet& cpuDescriptorSet, LowLevelContextDescriptorPoolVk& descriptorPool);
    void DestroyPool(LowLevelContextDescriptorPoolVk& descriptorPool);
    void ClearDescriptorSetWriteData();
    void ResizeDescriptorSetWriteData();

    uint32_t bufferingCount_ { 0 };
    LowLevelContextDescriptorPoolVk descriptorPool_[DESCRIPTOR_SET_INDEX_TYPE_COUNT];

    struct PendingDeallocations {
        uint64_t frameIndex { 0 };
        LowLevelContextDescriptorPoolVk descriptorPool;
    };
    BASE_NS::vector<PendingDeallocations> pendingDeallocations_;

    struct OneFrameDescriptorNeed {
        static constexpr uint32_t DESCRIPTOR_ARRAY_SIZE = CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1;
        uint8_t descriptorCount[DESCRIPTOR_ARRAY_SIZE] { 0 };
    };
    OneFrameDescriptorNeed oneFrameDescriptorNeed_;

    LowLevelDescriptorCountsVk defaultLowLevelDescriptorSetMemoryStoreVk_;

    // use same vector, so we do not re-create the same with every reset
    // the memory allocation is small
    BASE_NS::vector<VkDescriptorPoolSize> descriptorPoolSizes_;

    LowLevelContextDescriptorWriteDataVk lowLevelDescriptorWriteData_;

    uint32_t oneFrameDescSetGeneration_ { 0u };
#if (RENDER_VALIDATION_ENABLED == 1)
    static constexpr uint32_t MAX_ONE_FRAME_GENERATION_IDX { 16u };
#endif
};
RENDER_END_NAMESPACE()

#endif // VULKAN_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_VK_H
