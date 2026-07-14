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

#include "node_context_descriptor_set_manager_mln.h"
#include "mln_log.h"
#include "util/log.h"

#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_resource_handle_util.h"
#include "maleoon/descriptor_set_manager_mln.h"
#include "maleoon/device_mln.h"
#include "maleoon/mln_convert.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

namespace {

MlnBindingType ToMlnBindingType(DescriptorType type)
{
    // CORE_DESCRIPTOR_TYPE and MLN_BINDING_TYPE share identical integer values (Vulkan-derived)
    return static_cast<MlnBindingType>(type);
}

}  // anonymous namespace

NodeContextDescriptorSetManagerMln::NodeContextDescriptorSetManagerMln(Device& device)
    : NodeContextDescriptorSetManager(device)
{
    bufferingCount_ = device_.GetCommandBufferingCount();
    MLN_LOG_INIT("NodeContextDescriptorSetManagerMln: initialized (bufferingCount=%u)", bufferingCount_);
}

NodeContextDescriptorSetManagerMln::~NodeContextDescriptorSetManagerMln()
{
    for (uint32_t typeIdx = 0; typeIdx < DESCRIPTOR_SET_INDEX_TYPE_COUNT; ++typeIdx) {
        DestroyGpuDescriptorSetArray(gpuDescriptorSets_[typeIdx]);
    }
    for (auto& pendingData : pendingOneFrameGpuData_) {
        DestroyGpuDescriptorSetArray(pendingData.gpuSets);
    }
}

void NodeContextDescriptorSetManagerMln::ResetAndReserve(const DescriptorCounts& descriptorCounts)
{
    NodeContextDescriptorSetManager::ResetAndReserve(descriptorCounts);
    if (maxSets_ > 0) {
        auto& gpuSets = gpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
        if (!gpuSets.empty()) {
            PendingOneFrameGpuData pendingData{};
            pendingData.frameIndex = device_.GetFrameCount();
            pendingData.gpuSets = BASE_NS::move(gpuSets);
            pendingOneFrameGpuData_.push_back(BASE_NS::move(pendingData));
            gpuSets.clear();
        }
    }
}

void NodeContextDescriptorSetManagerMln::BeginFrame()
{
    NodeContextDescriptorSetManager::BeginFrame();
    ReclaimAgedOneFrameGpuData();
    // One-frame descriptor sets are reset each frame
    DestroyOneFrameGpuData();
    cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME].clear();
}

void NodeContextDescriptorSetManagerMln::DestroyOneFrameGpuData()
{
    auto& gpuSets = gpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];
    if (gpuSets.empty()) {
        return;
    }
    PendingOneFrameGpuData pendingData{};
    pendingData.frameIndex = device_.GetFrameCount();
    pendingData.gpuSets = BASE_NS::move(gpuSets);
    pendingOneFrameGpuData_.push_back(BASE_NS::move(pendingData));
    gpuSets.clear();
}

void NodeContextDescriptorSetManagerMln::DestroyGpuDescriptorSetArray(
    BASE_NS::vector<LowLevelDescriptorSetMln>& gpuSets)
{
    const MlnDevice mlnDevice = static_cast<DeviceMln&>(device_).GetMlnDevice();
    for (auto& gpuSet : gpuSets) {
        for (uint32_t bi = 0; bi < bufferingCount_; ++bi) {
            if (gpuSet.bufferingSet[bi]) {
                MlnDestroyBindingSet(mlnDevice, gpuSet.bufferingSet[bi]);
            }
        }
        if (gpuSet.bindingLayout) {
            MlnDestroyBindingLayout(mlnDevice, gpuSet.bindingLayout);
        }
    }
    gpuSets.clear();
}

void NodeContextDescriptorSetManagerMln::ReclaimAgedOneFrameGpuData()
{
    if (pendingOneFrameGpuData_.empty()) {
        return;
    }

    const uint64_t minAge = static_cast<uint64_t>(bufferingCount_) + 1u;
    const uint64_t frameCount = device_.GetFrameCount();
    const uint64_t ageLimit = (frameCount < minAge) ? 0u : (frameCount - minAge);

    size_t writeIdx = 0;
    for (size_t readIdx = 0; readIdx < pendingOneFrameGpuData_.size(); ++readIdx) {
        auto& pendingData = pendingOneFrameGpuData_[readIdx];
        if (pendingData.frameIndex <= ageLimit) {
            DestroyGpuDescriptorSetArray(pendingData.gpuSets);
        } else {
            if (writeIdx != readIdx) {
                pendingOneFrameGpuData_[writeIdx] = BASE_NS::move(pendingOneFrameGpuData_[readIdx]);
            }
            ++writeIdx;
        }
    }
    pendingOneFrameGpuData_.resize(writeIdx);
}

RenderHandle NodeContextDescriptorSetManagerMln::CreateDescriptorSet(
    array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    return CreateDescriptorSetImpl(descriptorSetLayoutBindings, DESCRIPTOR_SET_INDEX_TYPE_STATIC);
}

RenderHandle NodeContextDescriptorSetManagerMln::CreateOneFrameDescriptorSet(
    array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    return CreateDescriptorSetImpl(descriptorSetLayoutBindings, DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME);
}

RenderHandle NodeContextDescriptorSetManagerMln::CreateDescriptorSetImpl(
    array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings, DescriptorSetIndexType indexType)
{
    auto& cpuSets = cpuDescriptorSets_[indexType];
    const uint32_t index = static_cast<uint32_t>(cpuSets.size());
    MLN_LOG_GRAPH("NodeContextDescriptorSetManagerMln::CreateDescriptorSetImpl (type=%u, idx=%u, bindings=%zu)",
        indexType,
        index,
        descriptorSetLayoutBindings.size());

    CpuDescriptorSet cpuSet{};
    cpuSet.currentGpuBufferingIndex = 0;
    cpuSet.gpuDescriptorSetCreated = false;
    cpuSet.isDirty = true;

    cpuSet.bindings.resize(descriptorSetLayoutBindings.size());
    uint32_t bufferCount = 0;
    uint32_t imageCount = 0;
    uint32_t samplerCount = 0;
    for (size_t i = 0; i < descriptorSetLayoutBindings.size(); ++i) {
        const auto& binding = descriptorSetLayoutBindings[i];
        cpuSet.bindings[i].binding = binding;
        switch (binding.descriptorType) {
            case CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                bufferCount += binding.descriptorCount;
                break;
            case CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            case CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                imageCount += binding.descriptorCount;
                break;
            case CORE_DESCRIPTOR_TYPE_SAMPLER:
                samplerCount += binding.descriptorCount;
                break;
            default:
                break;
        }
        if (IsDynamicDescriptor(binding.descriptorType)) {
            cpuSet.hasDynamicBarrierResources = true;
            cpuSet.dynamicOffsetDescriptors.push_back({});
        }
    }
    cpuSet.buffers.resize(bufferCount);
    cpuSet.images.resize(imageCount);
    cpuSet.samplers.resize(samplerCount);

    cpuSets.push_back(BASE_NS::move(cpuSet));

    // Create MlnBindingLayout from the descriptor set layout bindings
    LowLevelDescriptorSetMln gpuSet{};
    if (!descriptorSetLayoutBindings.empty()) {
        const MlnDevice mlnDevice = static_cast<DeviceMln&>(device_).GetMlnDevice();

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

        gpuSet.bindingLayout = MlnCreateBindingLayout(mlnDevice, &layoutDesc);
        if (!gpuSet.bindingLayout) {
            MLN_LOG_ERR("NodeContextDescriptorSetManagerMln: MlnCreateBindingLayout failed");
        }
    }
    gpuSet.dirtyFramesRemaining = bufferingCount_;
    gpuDescriptorSets_[indexType].push_back(gpuSet);

    const uint32_t additionalBits = (indexType == DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME) ? ONE_FRAME_DESC_SET_BIT : 0u;
    return RenderHandleUtil::CreateHandle(RenderHandleType::DESCRIPTOR_SET, index, 0, additionalBits);
}

bool NodeContextDescriptorSetManagerMln::UpdateDescriptorSetGpuHandle(const RenderHandle handle)
{
    const uint32_t indexType = GetCpuDescriptorSetIndex(handle);
    if (indexType == ~0U) {
        return static_cast<DescriptorSetManagerMln&>(globalDescriptorSetMgr_).UpdateDescriptorSetGpuHandle(handle);
    }
    if (indexType >= DESCRIPTOR_SET_INDEX_TYPE_COUNT) {
        return false;
    }
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    auto& cpuSets = cpuDescriptorSets_[indexType];
    if (index >= cpuSets.size()) {
        return false;
    }

    auto& cpuSet = cpuSets[index];
    auto& gpuSets = gpuDescriptorSets_[indexType];
    if (index >= gpuSets.size()) {
        return false;
    }
    auto& gpuSet = gpuSets[index];

    // Create MlnBindingSet for all buffering slots if not yet created
    if (gpuSet.bindingLayout && !cpuSet.gpuDescriptorSetCreated) {
        const MlnDevice mlnDevice = static_cast<DeviceMln&>(device_).GetMlnDevice();
        for (uint32_t bi = 0; bi < bufferingCount_; ++bi) {
            gpuSet.bufferingSet[bi] = MlnCreateBindingSet(mlnDevice, gpuSet.bindingLayout, 0);
            if (!gpuSet.bufferingSet[bi]) {
                MLN_LOG_ERR("NodeContextDescriptorSetManagerMln: MlnCreateBindingSet failed (buffering=%u)", bi);
            }
        }
        cpuSet.gpuDescriptorSetCreated = true;
        // Ensure ALL buffering slots get written in subsequent frames.
        // Without this, rotating to an uninitialized slot causes black rendering.
        gpuSet.dirtyFramesRemaining = bufferingCount_;
        cpuSet.isDirty = true;
    }

    // Return dirty=true if either:
    // 1. CPU data was updated (isDirty), OR
    // 2. Not all buffering sets have been written to yet (dirtyFramesRemaining > 0).
    // Without (2), static descriptor sets (e.g. material textures) are only written to one
    // buffering slot - the other slot stays with null resource views -> SIGSEGV in Maleoon.
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

void NodeContextDescriptorSetManagerMln::UpdateCpuDescriptorSetPlatform(
    const DescriptorSetLayoutBindingResources& bindingResources)
{
    // Maleoon-specific: No platform conversion bindings needed (no YCbCr/OES)
}

void NodeContextDescriptorSetManagerMln::BeginBackendFrame()
{
    // Nothing special needed at backend frame begin for Maleoon.
    // One-frame pools are handled in BeginFrame().
    // Binding layouts are created at descriptor set creation time.
    // Binding sets are created lazily in UpdateDescriptorSetGpuHandle.
}

MlnBindingSet NodeContextDescriptorSetManagerMln::GetMlnBindingSet(const RenderHandle handle) const
{
    const uint32_t indexType = GetCpuDescriptorSetIndex(handle);
    if (indexType == ~0U) {
        return static_cast<const DescriptorSetManagerMln&>(globalDescriptorSetMgr_).GetMlnBindingSet(handle);
    }
    if (indexType >= DESCRIPTOR_SET_INDEX_TYPE_COUNT) {
        return MLN_NULL_HANDLE;
    }
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    const auto& gpuSets = gpuDescriptorSets_[indexType];
    if (index >= gpuSets.size()) {
        return MLN_NULL_HANDLE;
    }

    const auto& cpuSets = cpuDescriptorSets_[indexType];
    if (index >= cpuSets.size()) {
        return MLN_NULL_HANDLE;
    }

    const uint32_t bufferingIndex = cpuSets[index].currentGpuBufferingIndex;
    return gpuSets[index].bufferingSet[bufferingIndex];
}

const LowLevelDescriptorSetMln* NodeContextDescriptorSetManagerMln::GetLowLevelDescriptorSet(
    const RenderHandle handle) const
{
    const uint32_t indexType = GetCpuDescriptorSetIndex(handle);
    if (indexType >= DESCRIPTOR_SET_INDEX_TYPE_COUNT) {
        return nullptr;
    }
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    const auto& gpuSets = gpuDescriptorSets_[indexType];
    if (index >= gpuSets.size()) {
        return nullptr;
    }
    return &gpuSets[index];
}

RENDER_END_NAMESPACE()
