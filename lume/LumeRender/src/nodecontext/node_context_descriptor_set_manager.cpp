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

#include "node_context_descriptor_set_manager.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <base/containers/vector.h>
#include <base/math/mathf.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_resource_handle_util.h"
#include "nodecontext/pipeline_descriptor_set_binder.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
#if (RENDER_VALIDATION_ENABLED == 1)
void ReduceAndValidateDescriptorCounts(const DescriptorType descriptorType, const uint32_t descriptorCount,
    NodeContextDescriptorSetManager::DescriptorCountValidation& countValidation)
{
    auto& valRef = countValidation.typeToCount[static_cast<uint32_t>(descriptorType)];
    valRef -= descriptorCount;
    if (valRef < 0) {
        string typeName;
        if (descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
            typeName = "CORE_DESCRIPTOR_TYPE_SAMPLER";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
            typeName = "CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE) {
            typeName = "CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
            typeName = "CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) {
            typeName = "CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
            typeName = "CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            typeName = "CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
            typeName = "CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            typeName = "CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            typeName = "CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
            typeName = "CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
        } else if (descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE) {
            typeName = "CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE";
        }
        PLUGIN_LOG_E(
            "RENDER_VALIDATION: Not enough descriptors available (count: %i) (type: %s)", valRef, typeName.c_str());
    }
}
#endif

bool CopyAndProcessBuffers(const DescriptorSetLayoutBindingResources& src, const GpuQueue& gpuQueue,
    NodeContextDescriptorSetManager::CpuDescriptorSet& dst)
{
    const uint32_t maxCount = static_cast<uint32_t>(Math::min(src.buffers.size(), dst.buffers.size()));
    dst.buffers.clear();
    dst.buffers.insert(dst.buffers.end(), src.buffers.begin(), src.buffers.begin() + maxCount);
    uint32_t dynamicOffsetIndex = 0;
    bool valid = true;
    for (uint32_t idx = 0; idx < maxCount; ++idx) {
        auto& dstRef = dst.buffers[idx];
        dstRef.state.gpuQueue = gpuQueue;
        if (!RenderHandleUtil::IsGpuBuffer(dstRef.resource.handle)) {
            valid = false;
        }
        if (RenderHandleUtil::IsDynamicResource(dstRef.resource.handle)) {
            dst.hasDynamicBarrierResources = true;
        }
        if (NodeContextDescriptorSetManager::IsDynamicDescriptor(dstRef.binding.descriptorType)) {
            PLUGIN_ASSERT(dynamicOffsetIndex < (uint32_t)dst.dynamicOffsetDescriptors.size());
            dst.dynamicOffsetDescriptors[dynamicOffsetIndex++] = dstRef.resource.handle;
        }
    }
    return valid;
}

bool CopyAndProcessImages(const DescriptorSetLayoutBindingResources& src, const GpuQueue& gpuQueue,
    NodeContextDescriptorSetManager::CpuDescriptorSet& dst)
{
    const uint32_t maxCount = static_cast<uint32_t>(Math::min(src.images.size(), dst.images.size()));
    dst.images.clear();
    dst.images.insert(dst.images.end(), src.images.begin(), src.images.begin() + maxCount);
    bool valid = true;
    for (uint32_t idx = 0; idx < maxCount; ++idx) {
        auto& dstRef = dst.images[idx];
        dstRef.state.gpuQueue = gpuQueue;
        if (!RenderHandleUtil::IsGpuImage(dstRef.resource.handle)) {
            valid = false;
        }
        if (RenderHandleUtil::IsDynamicResource(dstRef.resource.handle)) {
            dst.hasDynamicBarrierResources = true;
        }
        if (dstRef.additionalFlags & CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT) {
            dst.hasImmutableSamplers = true;
        }
        if (RenderHandleUtil::IsPlatformConversionResource(dstRef.resource.handle)) {
            if (dstRef.binding.descriptorType == DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                dst.hasPlatformConversionBindings = true;
            }
#if (RENDER_VALIDATION_ENABLED == 1)
            if (dstRef.binding.descriptorType != DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                PLUGIN_LOG_ONCE_W("core_validation_plat_conv_desc_set",
                    "RENDER_VALIDATION: automatic platform conversion only supported for "
                    "CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER");
            }
            if (dstRef.binding.descriptorCount > 1) {
                PLUGIN_LOG_ONCE_W("core_validation_plat_conv_desc_set_arr",
                    "RENDER_VALIDATION: array of platform special image formats not supported");
            }
#endif
        }
    }
    return valid;
}

bool CopyAndProcessSamplers(
    const DescriptorSetLayoutBindingResources& src, NodeContextDescriptorSetManager::CpuDescriptorSet& dst)
{
    const uint32_t maxCount = static_cast<uint32_t>(Math::min(src.samplers.size(), dst.samplers.size()));
    dst.samplers.clear();
    dst.samplers.insert(dst.samplers.end(), src.samplers.begin(), src.samplers.begin() + maxCount);
    bool valid = true;
    for (uint32_t idx = 0; idx < maxCount; ++idx) {
        auto& dstRef = dst.samplers[idx];
        if (!RenderHandleUtil::IsGpuSampler(dstRef.resource.handle)) {
            valid = false;
        }
        if (dstRef.additionalFlags & CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT) {
            dst.hasImmutableSamplers = true;
        }
    }
    return valid;
}
} // namespace

void NodeContextDescriptorSetManager::ResetAndReserve(const DescriptorCounts& descriptorCounts)
{
    // method
    // prepare and create descriptor set pool
    maxSets_ = 0;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
    cpuDescriptorSets.clear();
#if (RENDER_VALIDATION_ENABLED == 1)
    descriptorCountValidation_ = {};
#endif
    for (const auto& ref : descriptorCounts.counts) {
        maxSets_ += ref.count;
#if (RENDER_VALIDATION_ENABLED == 1)
        auto& valRef = descriptorCountValidation_.typeToCount[static_cast<uint32_t>(ref.type)];
        valRef += static_cast<int32_t>(ref.count);
#endif
    }

    if (maxSets_ > 0) {
        // usually there are less descriptor sets than max sets count
        // due to maxsets count has been calculated for single descriptors
        // (one descriptor set has multiple descriptors)
        const uint32_t reserveCount = maxSets_ / 2u;
        cpuDescriptorSets.reserve(reserveCount);
    }
}

void NodeContextDescriptorSetManager::ResetAndReserve(const BASE_NS::array_view<DescriptorCounts> descriptorCounts)
{
    DescriptorCounts dc;
    dc.counts.reserve(descriptorCounts.size());
    for (size_t idx = 0; idx < descriptorCounts.size(); idx++) {
        const auto& ref = descriptorCounts[idx].counts;
        if (!ref.empty()) {
            dc.counts.insert(dc.counts.end(), ref.begin(), ref.end());
        }
    }
    ResetAndReserve(dc);
}

void NodeContextDescriptorSetManager::BeginFrame()
{
    cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME].clear();
    hasPlatformConversionBindings_ = false;
}

vector<RenderHandle> NodeContextDescriptorSetManager::CreateDescriptorSets(
    const array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings)
{
    vector<RenderHandle> handles;
    handles.reserve(descriptorSetsLayoutBindings.size());
    for (const auto& ref : descriptorSetsLayoutBindings) {
#if (RENDER_VALIDATION_ENABLED == 1)
        for (const auto& bindingRef : ref.binding) {
            ReduceAndValidateDescriptorCounts(
                bindingRef.descriptorType, bindingRef.descriptorCount, descriptorCountValidation_);
        }
#endif
        handles.push_back(CreateDescriptorSet(ref.binding));
    }
    return handles;
}

RenderHandle NodeContextDescriptorSetManager::CreateDescriptorSet(
    const uint32_t set, const PipelineLayout& pipelineLayout)
{
    if (set < pipelineLayout.descriptorSetCount) {
        const auto& ref = pipelineLayout.descriptorSetLayouts[set];
        if (ref.set == set) {
#if (RENDER_VALIDATION_ENABLED == 1)
            for (const auto& bindingRef : ref.bindings) {
                ReduceAndValidateDescriptorCounts(
                    bindingRef.descriptorType, bindingRef.descriptorCount, descriptorCountValidation_);
            }
#endif
            return CreateDescriptorSet(ref.bindings);
        } else {
            PLUGIN_LOG_E("CreateDescriptorSet: sets needs to be sorted in PipelineLayout");
        }
    }
    PLUGIN_LOG_E("set index needs to be valid in PipelineLayout");
    return {};
}

vector<RenderHandle> NodeContextDescriptorSetManager::CreateOneFrameDescriptorSets(
    const array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings)
{
    vector<RenderHandle> handles;
    handles.reserve(descriptorSetsLayoutBindings.size());
    for (const auto& ref : descriptorSetsLayoutBindings) {
        handles.push_back(CreateOneFrameDescriptorSet(ref.binding));
    }
    return handles;
}

RenderHandle NodeContextDescriptorSetManager::CreateOneFrameDescriptorSet(
    const uint32_t set, const PipelineLayout& pipelineLayout)
{
    if (set < pipelineLayout.descriptorSetCount) {
        const auto& ref = pipelineLayout.descriptorSetLayouts[set];
        if (ref.set == set) {
            return CreateOneFrameDescriptorSet(ref.bindings);
        } else {
            PLUGIN_LOG_E("CreateOneFrameDescriptorSet: sets needs to be sorted in PipelineLayout");
        }
    }
    PLUGIN_LOG_E("set index needs to be valid in PipelineLayout");
    return {};
}

IDescriptorSetBinder::Ptr NodeContextDescriptorSetManager::CreateDescriptorSetBinder(
    const RenderHandle handle, const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    return IDescriptorSetBinder::Ptr { new DescriptorSetBinder(handle, descriptorSetLayoutBindings) };
}

IPipelineDescriptorSetBinder::Ptr NodeContextDescriptorSetManager::CreatePipelineDescriptorSetBinder(
    const PipelineLayout& pipelineLayout)
{
    DescriptorSetLayoutBindings descriptorSetLayoutBindings[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT];
    RenderHandle handles[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT];
    for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
        if (pipelineLayout.descriptorSetLayouts[idx].set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
            descriptorSetLayoutBindings[idx] = { pipelineLayout.descriptorSetLayouts[idx].bindings };
            handles[idx] = CreateDescriptorSet(descriptorSetLayoutBindings[idx].binding);
#if (RENDER_VALIDATION_ENABLED == 1)
            for (const auto& bindingRef : descriptorSetLayoutBindings[idx].binding) {
                ReduceAndValidateDescriptorCounts(
                    bindingRef.descriptorType, bindingRef.descriptorCount, descriptorCountValidation_);
            }
#endif
        }
    }
    // pass max amount to binder, it will check validity of sets and their set indices
    return IPipelineDescriptorSetBinder::Ptr { new PipelineDescriptorSetBinder(
        pipelineLayout, handles, descriptorSetLayoutBindings) };
}

IPipelineDescriptorSetBinder::Ptr NodeContextDescriptorSetManager::CreatePipelineDescriptorSetBinder(
    const PipelineLayout& pipelineLayout, const array_view<const RenderHandle> handles,
    const array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings)
{
    return IPipelineDescriptorSetBinder::Ptr { new PipelineDescriptorSetBinder(
        pipelineLayout, handles, descriptorSetsLayoutBindings) };
}

DescriptorSetLayoutBindingResources NodeContextDescriptorSetManager::GetCpuDescriptorSetData(
    const RenderHandle handle) const
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                            : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    const auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
    return GetCpuDescriptorSetDataImpl(arrayIndex, cpuDescSets);
}

DynamicOffsetDescriptors NodeContextDescriptorSetManager::GetDynamicOffsetDescriptors(const RenderHandle handle) const
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                            : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    const auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
    return GetDynamicOffsetDescriptorsImpl(arrayIndex, cpuDescSets);
}

bool NodeContextDescriptorSetManager::HasDynamicBarrierResources(const RenderHandle handle) const
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                            : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    const auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
    return HasDynamicBarrierResourcesImpl(arrayIndex, cpuDescSets);
}

uint32_t NodeContextDescriptorSetManager::GetDynamicOffsetDescriptorCount(const RenderHandle handle) const
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                            : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    const auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
    return GetDynamicOffsetDescriptorCountImpl(arrayIndex, cpuDescSets);
}

bool NodeContextDescriptorSetManager::HasPlatformBufferBindings(const RenderHandle handle) const
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                            : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    const auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
    return HasPlatformBufferBindingsImpl(arrayIndex, cpuDescSets);
}

bool NodeContextDescriptorSetManager::UpdateCpuDescriptorSet(
    const RenderHandle handle, const DescriptorSetLayoutBindingResources& bindingResources, const GpuQueue& gpuQueue)
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit == ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                            : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
    return UpdateCpuDescriptorSetImpl(arrayIndex, bindingResources, gpuQueue, cpuDescSets);
}

bool NodeContextDescriptorSetManager::UpdateCpuDescriptorSetImpl(const uint32_t index,
    const DescriptorSetLayoutBindingResources& bindingResources, const GpuQueue& gpuQueue,
    vector<CpuDescriptorSet>& cpuDescriptorSets)
{
    bool valid = true;
    if (index < (uint32_t)cpuDescriptorSets.size()) {
        auto& refCpuSet = cpuDescriptorSets[index];
#if (RENDER_VALIDATION_ENABLED == 1)
        if (refCpuSet.bindings.size() != bindingResources.bindings.size()) {
            PLUGIN_LOG_E("RENDER_VALIDATION: sizes must match; update all bindings always in a single set");
        }
#endif

        refCpuSet.isDirty = true;
        refCpuSet.hasDynamicBarrierResources = false;
        refCpuSet.hasPlatformConversionBindings = false;
        refCpuSet.hasImmutableSamplers = false;

        // NOTE: GPU queue patching could be moved to render graph
        // copy from src to dst and check flags
        valid = valid && CopyAndProcessBuffers(bindingResources, gpuQueue, refCpuSet);
        valid = valid && CopyAndProcessImages(bindingResources, gpuQueue, refCpuSet);
        // samplers don't have state and dynamic resources, but we check for immutable samplers
        valid = valid && CopyAndProcessSamplers(bindingResources, refCpuSet);

        for (size_t idx = 0; idx < bindingResources.bindings.size(); ++idx) {
            const DescriptorSetLayoutBindingResource& refBinding = bindingResources.bindings[idx];
            // the actual binding index is not important here (refBinding.binding.binding)
            PLUGIN_ASSERT(idx < refCpuSet.bindings.size());
            DescriptorSetLayoutBindingResource& refCpuBinding = refCpuSet.bindings[idx];
            refCpuBinding.resourceIndex = refBinding.resourceIndex;
        }
        hasPlatformConversionBindings_ = (hasPlatformConversionBindings_ || refCpuSet.hasPlatformConversionBindings);

        // update platform data
        UpdateCpuDescriptorSetPlatform(bindingResources);
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle to UpdateCpuDescriptorSet");
#endif
    }
    return valid;
}

DescriptorSetLayoutBindingResources NodeContextDescriptorSetManager::GetCpuDescriptorSetDataImpl(
    const uint32_t index, const vector<CpuDescriptorSet>& cpuDescriptorSet) const
{
    if (index < cpuDescriptorSet.size()) {
        return DescriptorSetLayoutBindingResources {
            cpuDescriptorSet[index].bindings,
            cpuDescriptorSet[index].buffers,
            cpuDescriptorSet[index].images,
            cpuDescriptorSet[index].samplers,
        };
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle to GetCpuDescriptorSetData");
#endif
        return {};
    }
}

DynamicOffsetDescriptors NodeContextDescriptorSetManager::GetDynamicOffsetDescriptorsImpl(
    const uint32_t index, const vector<CpuDescriptorSet>& cpuDescriptorSet) const
{
    if (index < cpuDescriptorSet.size()) {
        return DynamicOffsetDescriptors {
            array_view<const RenderHandle>(cpuDescriptorSet[index].dynamicOffsetDescriptors.data(),
                cpuDescriptorSet[index].dynamicOffsetDescriptors.size()),
        };
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle to GetDynamicOffsetDescriptors");
#endif
        return {};
    }
}

bool NodeContextDescriptorSetManager::HasDynamicBarrierResourcesImpl(
    const uint32_t index, const vector<CpuDescriptorSet>& cpuDescriptorSet) const
{
    if (index < (uint32_t)cpuDescriptorSet.size()) {
        return cpuDescriptorSet[index].hasDynamicBarrierResources;
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle to HasDynamicBarrierResources");
#endif
        return false;
    }
}

bool NodeContextDescriptorSetManager::HasPlatformBufferBindingsImpl(
    const uint32_t index, const vector<CpuDescriptorSet>& cpuDescriptorSet) const
{
    if (index < (uint32_t)cpuDescriptorSet.size()) {
        return cpuDescriptorSet[index].hasPlatformConversionBindings;
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle to HasPlatformBufferBindingsImpl");
#endif
        return false;
    }
}

uint32_t NodeContextDescriptorSetManager::GetDynamicOffsetDescriptorCountImpl(
    const uint32_t index, const vector<CpuDescriptorSet>& cpuDescriptorSet) const
{
    if (index < (uint32_t)cpuDescriptorSet.size()) {
        return (uint32_t)cpuDescriptorSet[index].dynamicOffsetDescriptors.size();
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle to GetDynamicOffsetDescriptorCount");
#endif
        return 0;
    }
}

#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
void NodeContextDescriptorSetManager::SetValidationDebugName(const string_view debugName)
{
    debugName_ = debugName;
}
#endif
RENDER_END_NAMESPACE()
