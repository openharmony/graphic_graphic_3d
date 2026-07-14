/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "descriptor_set_manager_mln.h"

#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_resource_handle_util.h"
#include "maleoon/device_mln.h"
#include "maleoon/mln_convert.h"
#include "mln_log.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

namespace {

MlnBindingType ToMlnBindingType(DescriptorType type)
{
    // CORE_DESCRIPTOR_TYPE and MLN_BINDING_TYPE share identical integer values (Vulkan-derived)
    return static_cast<MlnBindingType>(type);
}

}  // anonymous namespace

DescriptorSetManagerMln::DescriptorSetManagerMln(Device& device) : DescriptorSetManager(device)
{
    mlnDevice_ = static_cast<DeviceMln&>(device_).GetMlnDevice();
    bufferingCount_ = device_.GetCommandBufferingCount();
    MLN_LOG_INIT("DescriptorSetManagerMln: initialized (bufferingCount=%u)", bufferingCount_);
}

DescriptorSetManagerMln::~DescriptorSetManagerMln()
{
    BASE_NS::vector<MlnBindingLayout> destroyedLayouts;
    for (auto& gpuSetArray : gpuDescriptorSets_) {
        destroyedLayouts.clear();
        for (auto& gpuSet : gpuSetArray) {
            for (uint32_t bi = 0; bi < bufferingCount_; ++bi) {
                if (gpuSet.bufferingSet[bi]) {
                    MlnDestroyBindingSet(mlnDevice_, gpuSet.bufferingSet[bi]);
                }
            }
            if (gpuSet.bindingLayout) {
                bool layoutAlreadyDestroyed = false;
                for (auto layout : destroyedLayouts) {
                    if (layout == gpuSet.bindingLayout) {
                        layoutAlreadyDestroyed = true;
                        break;
                    }
                }
                if (!layoutAlreadyDestroyed) {
                    MlnDestroyBindingLayout(mlnDevice_, gpuSet.bindingLayout);
                    destroyedLayouts.push_back(gpuSet.bindingLayout);
                }
            }
        }
    }
}

void DescriptorSetManagerMln::BeginFrame()
{
    DescriptorSetManager::BeginFrame();
}

void DescriptorSetManagerMln::BeginBackendFrame()
{
    // Reset per-frame write lock on all global descriptor sets.
    // Without this, global descriptor sets can only be written in the first frame
    // (frameWriteLocked stays true forever), preventing sampler/resource updates.
    // Matches Vulkan: node_context_descriptor_set_manager_vk.cpp:441
    for (uint32_t idx = 0; idx < descriptorSets_.size(); ++idx) {
        GlobalDescriptorSetBase* descriptorSetBase = descriptorSets_[idx].get();
        if (!descriptorSetBase) {
            continue;
        }
        for (auto& ref : descriptorSetBase->data) {
            ref.frameWriteLocked = false;
        }
    }
}

void DescriptorSetManagerMln::CreateDescriptorSets(const uint32_t arrayIndex, const uint32_t descriptorSetCount,
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    MLN_LOG_GRAPH("DescriptorSetManagerMln::CreateDescriptorSets (arrayIdx=%u, count=%u, bindings=%zu)",
        arrayIndex,
        descriptorSetCount,
        descriptorSetLayoutBindings.size());
    // Ensure gpuDescriptorSets_ is sized to match base class descriptorSets_
    if (arrayIndex >= gpuDescriptorSets_.size()) {
        gpuDescriptorSets_.resize(arrayIndex + 1);
    }

    auto& gpuSetArray = gpuDescriptorSets_[arrayIndex];
    gpuSetArray.reserve(gpuSetArray.size() + descriptorSetCount);

    // Create MlnBindingLayout from the descriptor set layout bindings
    MlnBindingLayout bindingLayout = MLN_NULL_HANDLE;
    if (!descriptorSetLayoutBindings.empty()) {
        vector<MlnBindingSlot> slots(descriptorSetLayoutBindings.size());
        for (size_t i = 0; i < descriptorSetLayoutBindings.size(); ++i) {
            const auto& binding = descriptorSetLayoutBindings[i];
            slots[i].slot = binding.binding;
            slots[i].type = ToMlnBindingType(binding.descriptorType);
            slots[i].slotCount = binding.descriptorCount;
            slots[i].stageFlags = static_cast<MlnShaderStageFlags>(ToMlnShaderStageFlagBits(binding.shaderStageFlags));
            slots[i].immutableSamplers = nullptr;
        }

        MlnBindingLayoutDescriptor layoutDesc{};
        layoutDesc.flags = 0;
        layoutDesc.bindingCount = static_cast<uint32_t>(slots.size());
        layoutDesc.bindings = slots.data();
        layoutDesc.bindingFlags = nullptr;

        bindingLayout = MlnCreateBindingLayout(mlnDevice_, &layoutDesc);
        if (!bindingLayout) {
            MLN_LOG_ERR("DescriptorSetManagerMln: MlnCreateBindingLayout failed");
        }
    }

    // Initialize CpuDescriptorSet for each global descriptor set
    // (matches Vulkan/GLES backends - without this, bindings vector stays empty
    //  and UpdateCpuDescriptorSetImpl crashes writing to null data pointer)
    PLUGIN_ASSERT((arrayIndex < descriptorSets_.size()) && (descriptorSets_[arrayIndex]));
    if ((arrayIndex < descriptorSets_.size()) && (descriptorSets_[arrayIndex])) {
        GlobalDescriptorSetBase* cpuData = descriptorSets_[arrayIndex].get();
        PLUGIN_ASSERT(cpuData->data.size() == descriptorSetCount);

        for (uint32_t i = 0; i < descriptorSetCount; ++i) {
            // Populate CpuDescriptorSet with binding layout info
            CpuDescriptorSet newSet;
            newSet.bindings.reserve(descriptorSetLayoutBindings.size());
            uint32_t dynamicOffsetCount = 0;
            LowLevelDescriptorCounts descriptorCounts;
            for (const auto& refBinding : descriptorSetLayoutBindings) {
                newSet.bindings.push_back({refBinding, {}});
                NodeContextDescriptorSetManager::IncreaseDescriptorSetCounts(
                    refBinding, descriptorCounts, dynamicOffsetCount);
            }
            newSet.buffers.resize(descriptorCounts.bufferCount);
            newSet.images.resize(descriptorCounts.imageCount);
            newSet.samplers.resize(descriptorCounts.samplerCount);
            newSet.dynamicOffsetDescriptors.resize(dynamicOffsetCount);

            cpuData->data[i].cpuDescriptorSet = BASE_NS::move(newSet);

            // Create Maleoon GPU objects
            LowLevelGlobalDescriptorSetMln gpuSet{};
            gpuSet.bindingLayout = bindingLayout;
            gpuSet.dirtyFramesRemaining = bufferingCount_;
            if (bindingLayout) {
                for (uint32_t bi = 0; bi < bufferingCount_; ++bi) {
                    gpuSet.bufferingSet[bi] = MlnCreateBindingSet(mlnDevice_, bindingLayout, 0);
                    if (!gpuSet.bufferingSet[bi]) {
                        MLN_LOG_ERR("DescriptorSetManagerMln: MlnCreateBindingSet failed (buffering=%u)", bi);
                    }
                }
            }
            gpuSetArray.push_back(gpuSet);
        }
    }
}

bool DescriptorSetManagerMln::UpdateDescriptorSetGpuHandle(const RenderHandle& handle)
{
    // Global descriptor set handle packs:
    // indexPart -> global descriptor array index
    // additionalIndexPart -> set index inside the global descriptor array
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t setIndex = RenderHandleUtil::GetAdditionalIndexPart(handle);

    if (arrayIndex >= descriptorSets_.size() || arrayIndex >= gpuDescriptorSets_.size()) {
        return false;
    }

    auto& baseSetArray = descriptorSets_[arrayIndex];
    if (!baseSetArray) {
        return false;
    }

    auto& gpuSetArray = gpuDescriptorSets_[arrayIndex];
    if (setIndex >= baseSetArray->data.size() || setIndex >= gpuSetArray.size()) {
        return false;
    }

    auto& globalData = baseSetArray->data[setIndex];
    auto& cpuSet = globalData.cpuDescriptorSet;
    auto& gpuSet = gpuSetArray[setIndex];

    if (cpuSet.isDirty || gpuSet.dirtyFramesRemaining > 0) {
        cpuSet.currentGpuBufferingIndex = (cpuSet.currentGpuBufferingIndex + 1u) % bufferingCount_;
        cpuSet.isDirty = false;
        if (gpuSet.dirtyFramesRemaining > 0) {
            gpuSet.dirtyFramesRemaining--;
        }
        return true;  // needs GPU update
    }
    return false;
}

void DescriptorSetManagerMln::UpdateCpuDescriptorSetPlatform(
    const DescriptorSetLayoutBindingResources& bindingResources)
{
    // Maleoon-specific: No platform conversion bindings needed (no YCbCr/OES)
}

const LowLevelGlobalDescriptorSetMln* DescriptorSetManagerMln::GetDescriptorSet(const RenderHandle& handle) const
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t setIndex = RenderHandleUtil::GetAdditionalIndexPart(handle);

    if ((arrayIndex >= gpuDescriptorSets_.size()) || (arrayIndex >= descriptorSets_.size()) ||
        !descriptorSets_[arrayIndex]) {
        return nullptr;
    }

    const auto& gpuSetArray = gpuDescriptorSets_[arrayIndex];
    if (setIndex >= gpuSetArray.size() || setIndex >= descriptorSets_[arrayIndex]->data.size()) {
        return nullptr;
    }

    return &gpuSetArray[setIndex];
}

MlnBindingSet DescriptorSetManagerMln::GetMlnBindingSet(const RenderHandle& handle) const
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t setIndex = RenderHandleUtil::GetAdditionalIndexPart(handle);

    if ((arrayIndex >= descriptorSets_.size()) || !descriptorSets_[arrayIndex] ||
        (arrayIndex >= gpuDescriptorSets_.size())) {
        return MLN_NULL_HANDLE;
    }

    const auto& cpuSetArray = descriptorSets_[arrayIndex]->data;
    const auto& gpuSetArray = gpuDescriptorSets_[arrayIndex];
    if ((setIndex >= cpuSetArray.size()) || (setIndex >= gpuSetArray.size())) {
        return MLN_NULL_HANDLE;
    }

    const uint32_t bufferingIndex = cpuSetArray[setIndex].cpuDescriptorSet.currentGpuBufferingIndex;
    return gpuSetArray[setIndex].bufferingSet[bufferingIndex];
}

RENDER_END_NAMESPACE()
