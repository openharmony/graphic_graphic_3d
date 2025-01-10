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

#include <cstddef>
#include <cstdint>

#include <base/containers/vector.h>
#include <base/math/mathf.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
#include "nodecontext/pipeline_descriptor_set_binder.h"
#include "resource_handle_impl.h"
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

constexpr uint64_t RENDER_HANDLE_REMAPPABLE_MASK_ID { (
    uint64_t(RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_SHALLOW_RESOURCE |
             RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_SWAPCHAIN_RESOURCE |
             RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_PLATFORM_CONVERSION)
    << RenderHandleUtil::RES_HANDLE_ADDITIONAL_INFO_SHIFT) };

inline bool IsTheSameBufferBinding(const BindableBuffer& src, const BindableBuffer& dst,
    const EngineResourceHandle& srcHandle, const EngineResourceHandle& dstHandle)
{
    return (src.handle == dst.handle) && (srcHandle.id == dstHandle.id) && (src.byteOffset == dst.byteOffset) &&
           (src.byteSize == dst.byteSize) && ((src.handle.id & RENDER_HANDLE_REMAPPABLE_MASK_ID) == 0);
}

inline bool IsTheSameImageBinding(const BindableImage& src, const BindableImage& dst,
    const EngineResourceHandle& srcHandle, const EngineResourceHandle& dstHandle,
    const EngineResourceHandle& srcSamplerHandle, const EngineResourceHandle& dstSamplerHandle)
{
    return (src.handle == dst.handle) && (srcHandle.id == dstHandle.id) && (src.mip == dst.mip) &&
           (src.layer == dst.layer) && (src.imageLayout == dst.imageLayout) &&
           (srcSamplerHandle.id == dstSamplerHandle.id) && ((src.handle.id & RENDER_HANDLE_REMAPPABLE_MASK_ID) == 0);
}

inline bool IsTheSameSamplerBinding(const BindableSampler& src, const BindableSampler& dst,
    const EngineResourceHandle& srcHandle, const EngineResourceHandle& dstHandle)
{
    return (src.handle == dst.handle) && (srcHandle.id == dstHandle.id) &&
           ((src.handle.id & RENDER_HANDLE_REMAPPABLE_MASK_ID) == 0);
}

DescriptorSetUpdateInfoFlags CopyAndProcessBuffers(const GpuResourceManager& gpuResourceMgr,
    const DescriptorSetLayoutBindingResources& src, const GpuQueue& gpuQueue, CpuDescriptorSet& dst)
{
    const auto maxCount = static_cast<uint32_t>(src.buffers.size());
    dst.buffers.resize(maxCount);
    uint32_t dynamicOffsetIndex = 0U;
    DescriptorSetUpdateInfoFlags updateFlags = 0U;
    for (uint32_t idx = 0; idx < maxCount; ++idx) {
        const auto& srcRef = src.buffers[idx];
        auto& dstRef = dst.buffers[idx].desc;
        auto& dstHandle = dst.buffers[idx].handle;
        // we need to get the correct (the latest) generation id from the gpu resource manager
        const EngineResourceHandle srcHandle = gpuResourceMgr.GetGpuHandle(srcRef.resource.handle);
        if ((!IsTheSameBufferBinding(srcRef.resource, dstRef.resource, srcHandle, dstHandle))) {
            updateFlags |= DescriptorSetUpdateInfoFlagBits::DESCRIPTOR_SET_UPDATE_INFO_NEW_BIT;
        }
        dstRef = srcRef;
        dstHandle = srcHandle;
        dstRef.state.gpuQueue = gpuQueue;
        if (!RenderHandleUtil::IsGpuBuffer(dstRef.resource.handle)) {
            updateFlags |= DescriptorSetUpdateInfoFlagBits::DESCRIPTOR_SET_UPDATE_INFO_INVALID_BIT;
        }
        if (RenderHandleUtil::IsDynamicResource(dstRef.resource.handle)) {
            dst.hasDynamicBarrierResources = true;
        }
        if (NodeContextDescriptorSetManager::IsDynamicDescriptor(dstRef.binding.descriptorType)) {
            PLUGIN_ASSERT(dynamicOffsetIndex < (uint32_t)dst.dynamicOffsetDescriptors.size());
            dst.dynamicOffsetDescriptors[dynamicOffsetIndex++] = dstRef.resource.handle;
        }
    }
    return updateFlags;
}

DescriptorSetUpdateInfoFlags CopyAndProcessImages(const GpuResourceManager& gpuResourceMgr,
    const DescriptorSetLayoutBindingResources& src, const GpuQueue& gpuQueue, CpuDescriptorSet& dst)
{
    const auto maxCount = static_cast<uint32_t>(src.images.size());
    dst.images.resize(maxCount);
    DescriptorSetUpdateInfoFlags updateFlags = 0U;
    for (uint32_t idx = 0; idx < maxCount; ++idx) {
        const auto& srcRef = src.images[idx];
        auto& dstRef = dst.images[idx].desc;
        auto& dstHandle = dst.images[idx].handle;
        auto& dstSamplerHandle = dst.images[idx].samplerHandle;
        // we need to get the correct (the latest) generation id from the gpu resource manager
        const EngineResourceHandle srcHandle = gpuResourceMgr.GetGpuHandle(srcRef.resource.handle);
        const EngineResourceHandle srcSamplerHandle = gpuResourceMgr.GetGpuHandle(srcRef.resource.samplerHandle);
        if ((!IsTheSameImageBinding(
                srcRef.resource, dstRef.resource, srcHandle, dstHandle, srcSamplerHandle, dstSamplerHandle))) {
            updateFlags |= DescriptorSetUpdateInfoFlagBits::DESCRIPTOR_SET_UPDATE_INFO_NEW_BIT;
        }
        dstRef = srcRef;
        dstHandle = srcHandle;
        dstSamplerHandle = srcSamplerHandle;
        dstRef.state.gpuQueue = gpuQueue;
        if (!RenderHandleUtil::IsGpuImage(dstRef.resource.handle)) {
            updateFlags |= DescriptorSetUpdateInfoFlagBits::DESCRIPTOR_SET_UPDATE_INFO_INVALID_BIT;
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
    return updateFlags;
}

DescriptorSetUpdateInfoFlags CopyAndProcessSamplers(
    const GpuResourceManager& gpuResourceMgr, const DescriptorSetLayoutBindingResources& src, CpuDescriptorSet& dst)
{
    const auto maxCount = static_cast<uint32_t>(src.samplers.size());
    dst.samplers.resize(maxCount);
    DescriptorSetUpdateInfoFlags updateFlags = 0U;
    for (uint32_t idx = 0; idx < maxCount; ++idx) {
        const auto& srcRef = src.samplers[idx];
        auto& dstRef = dst.samplers[idx].desc;
        auto& dstHandle = dst.samplers[idx].handle;
        // we need to get the correct (the latest) generation id from the gpu resource manager
        const EngineResourceHandle srcHandle = gpuResourceMgr.GetGpuHandle(srcRef.resource.handle);
        // we need to get the correct (the latest) generation id from the gpu resource manager
        if ((!IsTheSameSamplerBinding(srcRef.resource, dstRef.resource, srcHandle, dstHandle))) {
            updateFlags |= DescriptorSetUpdateInfoFlagBits::DESCRIPTOR_SET_UPDATE_INFO_NEW_BIT;
        }
        dstRef = srcRef;
        dstHandle = srcHandle;

        if (!RenderHandleUtil::IsGpuSampler(dstRef.resource.handle)) {
            updateFlags |= DescriptorSetUpdateInfoFlagBits::DESCRIPTOR_SET_UPDATE_INFO_INVALID_BIT;
        }
        if (dstRef.additionalFlags & CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT) {
            dst.hasImmutableSamplers = true;
        }
    }
    return updateFlags;
}

DescriptorSetUpdateInfoFlags UpdateCpuDescriptorSetFunc(const GpuResourceManager& gpuResourceMgr,
    const DescriptorSetLayoutBindingResources& bindingResources, const GpuQueue& gpuQueue, CpuDescriptorSet& cpuSet,
    bool& hasPlatformConversionBindings)
{
    DescriptorSetUpdateInfoFlags updateFlags = 0U;
#if (RENDER_VALIDATION_ENABLED == 1)
    if (cpuSet.bindings.size() != bindingResources.bindings.size()) {
        PLUGIN_LOG_E("RENDER_VALIDATION: sizes must match; update all bindings always in a single set");
    }
#endif
    cpuSet.hasDynamicBarrierResources = false;
    cpuSet.hasPlatformConversionBindings = false;
    cpuSet.hasImmutableSamplers = false;
    cpuSet.isDirty = true;

    // NOTE: GPU queue patching could be moved to render graph
    // copy from src to dst and check flags
    updateFlags |= CopyAndProcessBuffers(gpuResourceMgr, bindingResources, gpuQueue, cpuSet);
    updateFlags |= CopyAndProcessImages(gpuResourceMgr, bindingResources, gpuQueue, cpuSet);
    // samplers don't have state and dynamic resources, but we check for immutable samplers
    updateFlags |= CopyAndProcessSamplers(gpuResourceMgr, bindingResources, cpuSet);

    // cpuSet.isDirty = ((updateFlags & DescriptorSetUpdateInfoFlagBits::DESCRIPTOR_SET_UPDATE_INFO_NEW_BIT) != 0);

    for (size_t idx = 0; idx < bindingResources.bindings.size(); ++idx) {
        const DescriptorSetLayoutBindingResource& refBinding = bindingResources.bindings[idx];
        // the actual binding index is not important here (refBinding.binding.binding)
        PLUGIN_ASSERT(idx < cpuSet.bindings.size());
        DescriptorSetLayoutBindingResource& refCpuBinding = cpuSet.bindings[idx];
        refCpuBinding.resourceIndex = refBinding.resourceIndex;
    }
    hasPlatformConversionBindings = (hasPlatformConversionBindings || cpuSet.hasPlatformConversionBindings);
    return updateFlags;
}
} // namespace

DescriptorSetManager::DescriptorSetManager(Device& device) : device_(device) {}

void DescriptorSetManager::BeginFrame()
{
    descriptorSetHandlesForUpdate_.clear();

    creationLocked_ = false;
}

void DescriptorSetManager::LockFrameCreation()
{
    creationLocked_ = true;
}

vector<RenderHandleReference> DescriptorSetManager::CreateDescriptorSets(const string_view name,
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings, const uint32_t descCount)
{
    if (creationLocked_) {
        PLUGIN_LOG_W("Global descriptor set can only be created in Init or PreExecute (%s)", string(name).c_str());
        return {};
    }
    vector<RenderHandleReference> createdDescriptorSets;
    if (!name.empty()) {
        uint32_t arrayIndex = ~0U;
        uint32_t generationCounter = 0U;
        {
            if (const auto iter = nameToIndex_.find(name); iter != nameToIndex_.cend()) {
                arrayIndex = iter->second;
                if (arrayIndex >= descriptorSets_.size()) {
#if (RENDER_VALIDATION_ENABLED == 1)
                    PLUGIN_LOG_W("RENDER_VALIDATION: Global descriptor set name/index missmatch.");
#endif
                    arrayIndex = ~0U;
                    PLUGIN_LOG_W("Overwriting named global descriptor set (name: %s)", string(name).c_str());
                }
            }
            if (arrayIndex == ~0U) {
                if (!availableHandles_.empty()) {
                    // re-use with generation counter
                    const RenderHandle reuseHandle = availableHandles_.back();
                    availableHandles_.pop_back();
                    arrayIndex = RenderHandleUtil::GetIndexPart(reuseHandle);
                    generationCounter = RenderHandleUtil::GetGenerationIndexPart(reuseHandle) + 1;
                }
                if (arrayIndex == ~0U) {
                    arrayIndex = static_cast<uint32_t>(descriptorSets_.size());
                    descriptorSets_.push_back({});
                }
            }

            // now, match our name to array index of vector
            nameToIndex_.insert_or_assign(name, arrayIndex);

            // now, create all similar descriptor sets for this global name with changing additional index
            if (arrayIndex < descriptorSets_.size()) {
                auto& ref = descriptorSets_[arrayIndex];
                if (!ref) {
                    ref = make_unique<GlobalDescriptorSetBase>();
                }
                if (ref) {
                    ref->name = name;
                    createdDescriptorSets.resize(descCount);

                    constexpr uint32_t additionalData = NodeContextDescriptorSetManager::GLOBAL_DESCRIPTOR_BIT;
                    ref->handles.resize(descCount);
                    ref->data.resize(descCount);
                    for (uint32_t idx = 0; idx < descCount; ++idx) {
                        const uint32_t additionalIndex = idx;
                        const RenderHandle rawHandle = RenderHandleUtil::CreateHandle(RenderHandleType::DESCRIPTOR_SET,
                            arrayIndex, generationCounter, additionalData, additionalIndex);
                        ref->data[idx].frameWriteLocked = false;
                        ref->data[idx].renderHandleReference = RenderHandleReference(
                            rawHandle, IRenderReferenceCounter::Ptr(new RenderReferenceCounter()));

                        ref->handles[idx] = rawHandle;
                        createdDescriptorSets[idx] = ref->data[idx].renderHandleReference;
                    }
                    CreateDescriptorSets(arrayIndex, descCount, descriptorSetLayoutBindings);
                }
            }
        }
    }

    return createdDescriptorSets;
}

RenderHandleReference DescriptorSetManager::CreateDescriptorSet(
    const string_view name, const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    const auto handles = CreateDescriptorSets(name, descriptorSetLayoutBindings, 1U);
    if (!handles.empty()) {
        return handles[0U];
    } else {
        return {};
    }
}

RenderHandle DescriptorSetManager::GetDescriptorSetHandle(const string_view name) const
{
    if (!creationLocked_) {
        PLUGIN_LOG_W("Fetch global descriptor set in ExecuteFrame (%s)", string(name).c_str());
        return {};
    }

    if (!name.empty()) {
        if (const auto iter = nameToIndex_.find(name); iter != nameToIndex_.cend()) {
            const uint32_t index = iter->second;
#if (RENDER_VALIDATION_ENABLED == 1)
            if (index >= descriptorSets_.size()) {
                PLUGIN_LOG_W("RENDER_VALIDATION: Global descriptor set name/index missmatch.");
            }
#endif
            if ((index < descriptorSets_.size()) && descriptorSets_[index] &&
                (!descriptorSets_[index]->handles.empty())) {
                // return the first
                return descriptorSets_[index]->handles[0U];
            }
        }
    }
    return {};
}

array_view<const RenderHandle> DescriptorSetManager::GetDescriptorSetHandles(const string_view name) const
{
    if (!creationLocked_) {
        PLUGIN_LOG_W("Fetch global descriptor sets in ExecuteFrame (%s)", string(name).c_str());
        return {};
    }

    if (!name.empty()) {
        if (const auto iter = nameToIndex_.find(name); iter != nameToIndex_.cend()) {
            const uint32_t index = iter->second;
#if (RENDER_VALIDATION_ENABLED == 1)
            if (index >= descriptorSets_.size()) {
                PLUGIN_LOG_W("RENDER_VALIDATION: Global descriptor set name/index missmatch.");
            }
#endif
            if ((index < descriptorSets_.size()) && descriptorSets_[index] &&
                (!descriptorSets_[index]->handles.empty())) {
                return descriptorSets_[index]->handles;
            }
        }
    }

    return {};
}

CpuDescriptorSet* DescriptorSetManager::GetCpuDescriptorSet(const RenderHandle& handle)
{
    PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(handle) == RenderHandleType::DESCRIPTOR_SET);

    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t additionalIndex = RenderHandleUtil::GetAdditionalIndexPart(handle);

    CpuDescriptorSet* cpuDescriptorSet = nullptr;
    {
        if ((index < descriptorSets_.size()) && descriptorSets_[index] &&
            (additionalIndex < descriptorSets_[index]->data.size())) {
            PLUGIN_ASSERT(descriptorSets_[index]->data.size() == descriptorSets_[index]->handles.size());

            cpuDescriptorSet = &descriptorSets_[index]->data[additionalIndex].cpuDescriptorSet;
        }
    }
    return cpuDescriptorSet;
}

DescriptorSetLayoutBindingResourcesHandler DescriptorSetManager::GetCpuDescriptorSetData(const RenderHandle& handle)
{
    if (const CpuDescriptorSet* cpuDescriptorSet = GetCpuDescriptorSet(handle); cpuDescriptorSet) {
        return DescriptorSetLayoutBindingResourcesHandler {
            cpuDescriptorSet->bindings,
            cpuDescriptorSet->buffers,
            cpuDescriptorSet->images,
            cpuDescriptorSet->samplers,
        };
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle to GetCpuDescriptorSetData");
#endif
        return {};
    }
}

DynamicOffsetDescriptors DescriptorSetManager::GetDynamicOffsetDescriptors(const RenderHandle& handle)
{
    if (const CpuDescriptorSet* cpuDescriptorSet = GetCpuDescriptorSet(handle); cpuDescriptorSet) {
        return DynamicOffsetDescriptors {
            array_view<const RenderHandle>(
                cpuDescriptorSet->dynamicOffsetDescriptors.data(), cpuDescriptorSet->dynamicOffsetDescriptors.size()),
        };
    } else {
        return {};
    }
}

bool DescriptorSetManager::HasDynamicBarrierResources(const RenderHandle& handle)
{
    // NOTE: this method cannot provide up-to-date information during ExecuteFrame
    // some render node task will update the dynamicity data in parallel

    if (const CpuDescriptorSet* cpuDescriptorSet = GetCpuDescriptorSet(handle); cpuDescriptorSet) {
        return cpuDescriptorSet->hasDynamicBarrierResources;
    } else {
        return false;
    }
}

uint32_t DescriptorSetManager::GetDynamicOffsetDescriptorCount(const RenderHandle& handle)
{
    if (const CpuDescriptorSet* cpuDescriptorSet = GetCpuDescriptorSet(handle); cpuDescriptorSet) {
        return static_cast<uint32_t>(cpuDescriptorSet->dynamicOffsetDescriptors.size());
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle to GetDynamicOffsetDescriptorCount");
#endif
        return 0U;
    }
}

DescriptorSetUpdateInfoFlags DescriptorSetManager::UpdateCpuDescriptorSet(
    const RenderHandle& handle, const DescriptorSetLayoutBindingResources& bindingResources, const GpuQueue& gpuQueue)
{
    PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(handle) == RenderHandleType::DESCRIPTOR_SET);

    // NOTE: needs locks for global data access
    // the local data, which is inside vector(s) do not need locks
    // the global vectors are only resized when new descriptor sets are created ->
    // and only from PreExecuteFrame (single thread)

    DescriptorSetUpdateInfoFlags updateFlags = 0U;
    constexpr uint32_t validUpdateFlags = DescriptorSetUpdateInfoFlagBits::DESCRIPTOR_SET_UPDATE_INFO_NEW_BIT;
    {
        const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
        const uint32_t additionalIndex = RenderHandleUtil::GetAdditionalIndexPart(handle);
        if ((index < descriptorSets_.size()) && descriptorSets_[index] &&
            (additionalIndex < descriptorSets_[index]->data.size())) {
            auto& ref = descriptorSets_[index]->data[additionalIndex];
            bool validWrite = false;
            {
                // lock global mutex for shared data and for the boolean inside vectors
                const auto lock = std::lock_guard(mutex_);

                if (!ref.frameWriteLocked) {
                    // NOTE: not used at the moment
                    bool hasPlatformConversionBindings = false;
                    updateFlags = UpdateCpuDescriptorSetFunc((const GpuResourceManager&)device_.GetGpuResourceManager(),
                        bindingResources, gpuQueue, ref.cpuDescriptorSet, hasPlatformConversionBindings);
                    // set for update, these needs to be locked
                    if ((updateFlags & 0xFFFFffffU) == validUpdateFlags) {
                        descriptorSetHandlesForUpdate_.push_back(handle);
                        UpdateCpuDescriptorSetPlatform(bindingResources);
                        // mark as write locked for this frame
                        ref.frameWriteLocked = true;
                    }
                    validWrite =
                        ((updateFlags & DescriptorSetUpdateInfoFlagBits::DESCRIPTOR_SET_UPDATE_INFO_INVALID_BIT) == 0U);
                }
            }
            // the following do not need locks
#if (RENDER_VALIDATION_ENABLED)
            if (!validWrite) {
                const string tmpName = string(to_string(handle.id)) + descriptorSets_[index]->name.c_str();
                PLUGIN_LOG_ONCE_W(tmpName, "RENDER_VALIDATION: Global descriptor set already updated this frame (%s)",
                    descriptorSets_[index]->name.c_str());
            }
#else
            PLUGIN_UNUSED(validWrite);
#endif
        }
    }
    return updateFlags;
}

array_view<const RenderHandle> DescriptorSetManager::GetUpdateDescriptorSetHandles() const
{
    return { descriptorSetHandlesForUpdate_.data(), descriptorSetHandlesForUpdate_.size() };
}

NodeContextDescriptorSetManager::NodeContextDescriptorSetManager(Device& device)
    : device_(device), globalDescriptorSetMgr_(device.GetDescriptorSetManager())
{}

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
    for (const auto& descriptorCount : descriptorCounts) {
        const auto& ref = descriptorCount.counts;
        if (!ref.empty()) {
            dc.counts.append(ref.begin(), ref.end());
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
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
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
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
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

vector<RenderHandleReference> NodeContextDescriptorSetManager::CreateGlobalDescriptorSets(
    const BASE_NS::string_view name,
    const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings,
    const uint32_t descriptorSetCount)
{
    if ((!name.empty()) && (!descriptorSetLayoutBindings.empty())) {
        return globalDescriptorSetMgr_.CreateDescriptorSets(name, descriptorSetLayoutBindings, descriptorSetCount);
    } else {
        return {};
    }
}

RenderHandleReference NodeContextDescriptorSetManager::CreateGlobalDescriptorSet(const BASE_NS::string_view name,
    const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    RenderHandleReference handle;
    if ((!name.empty()) && (!descriptorSetLayoutBindings.empty())) {
        handle = globalDescriptorSetMgr_.CreateDescriptorSet(name, descriptorSetLayoutBindings);
    }
    return handle;
}

RenderHandle NodeContextDescriptorSetManager::GetGlobalDescriptorSet(const BASE_NS::string_view name) const
{
    return globalDescriptorSetMgr_.GetDescriptorSetHandle(name);
}

array_view<const RenderHandle> NodeContextDescriptorSetManager::GetGlobalDescriptorSets(
    const BASE_NS::string_view name) const
{
    return globalDescriptorSetMgr_.GetDescriptorSetHandles(name);
}

IDescriptorSetBinder::Ptr NodeContextDescriptorSetManager::CreateDescriptorSetBinder(
    const RenderHandle handle, const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    return IDescriptorSetBinder::Ptr { new DescriptorSetBinder(handle, descriptorSetLayoutBindings) };
}

IDescriptorSetBinder::Ptr NodeContextDescriptorSetManager::CreateDescriptorSetBinder(
    const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    const RenderHandle handle = CreateDescriptorSet(descriptorSetLayoutBindings);
    return CreateDescriptorSetBinder(handle, descriptorSetLayoutBindings);
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

DescriptorSetLayoutBindingResourcesHandler NodeContextDescriptorSetManager::GetCpuDescriptorSetData(
    const RenderHandle handle) const
{
    const uint32_t descSetIdx = GetCpuDescriptorSetIndex(handle);
    if (descSetIdx == ~0U) {
        return globalDescriptorSetMgr_.GetCpuDescriptorSetData(handle);
    } else {
        PLUGIN_ASSERT(descSetIdx < DESCRIPTOR_SET_INDEX_TYPE_COUNT);
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
        return GetCpuDescriptorSetDataImpl(arrayIndex, cpuDescSets);
    }
}

DynamicOffsetDescriptors NodeContextDescriptorSetManager::GetDynamicOffsetDescriptors(const RenderHandle handle) const
{
    const uint32_t descSetIdx = GetCpuDescriptorSetIndex(handle);
    if (descSetIdx == ~0U) {
        return globalDescriptorSetMgr_.GetDynamicOffsetDescriptors(handle);
    } else {
        PLUGIN_ASSERT(descSetIdx < DESCRIPTOR_SET_INDEX_TYPE_COUNT);
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
        return GetDynamicOffsetDescriptorsImpl(arrayIndex, cpuDescSets);
    }
}

bool NodeContextDescriptorSetManager::HasDynamicBarrierResources(const RenderHandle handle) const
{
    const uint32_t descSetIdx = GetCpuDescriptorSetIndex(handle);
    if (descSetIdx == ~0U) {
        return globalDescriptorSetMgr_.HasDynamicBarrierResources(handle);
    } else {
        PLUGIN_ASSERT(descSetIdx < DESCRIPTOR_SET_INDEX_TYPE_COUNT);
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
        return HasDynamicBarrierResourcesImpl(arrayIndex, cpuDescSets);
    }
}

uint32_t NodeContextDescriptorSetManager::GetDynamicOffsetDescriptorCount(const RenderHandle handle) const
{
    const uint32_t descSetIdx = GetCpuDescriptorSetIndex(handle);
    if (descSetIdx == ~0U) {
        return globalDescriptorSetMgr_.GetDynamicOffsetDescriptorCount(handle);
    } else {
        PLUGIN_ASSERT(descSetIdx < DESCRIPTOR_SET_INDEX_TYPE_COUNT);
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
        return GetDynamicOffsetDescriptorCountImpl(arrayIndex, cpuDescSets);
    }
}

DescriptorSetUpdateInfoFlags NodeContextDescriptorSetManager::UpdateCpuDescriptorSet(
    const RenderHandle handle, const DescriptorSetLayoutBindingResources& bindingResources, const GpuQueue& gpuQueue)
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t descSetIdx = GetCpuDescriptorSetIndex(handle);
    if (descSetIdx == ~0U) {
        return globalDescriptorSetMgr_.UpdateCpuDescriptorSet(handle, bindingResources, gpuQueue);
    } else {
        PLUGIN_ASSERT(descSetIdx < DESCRIPTOR_SET_INDEX_TYPE_COUNT);
        auto& cpuDescSets = cpuDescriptorSets_[descSetIdx];
        return UpdateCpuDescriptorSetImpl(arrayIndex, bindingResources, gpuQueue, cpuDescSets);
    }
}

DescriptorSetUpdateInfoFlags NodeContextDescriptorSetManager::UpdateCpuDescriptorSetImpl(const uint32_t index,
    const DescriptorSetLayoutBindingResources& bindingResources, const GpuQueue& gpuQueue,
    vector<CpuDescriptorSet>& cpuDescriptorSets)
{
    DescriptorSetUpdateInfoFlags updateFlags = 0U;
    if (index < (uint32_t)cpuDescriptorSets.size()) {
        auto& refCpuSet = cpuDescriptorSets[index];
        updateFlags = UpdateCpuDescriptorSetFunc((const GpuResourceManager&)device_.GetGpuResourceManager(),
            bindingResources, gpuQueue, refCpuSet, hasPlatformConversionBindings_);
        // update platform data for local
        UpdateCpuDescriptorSetPlatform(bindingResources);
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle to UpdateCpuDescriptorSet");
#endif
    }
    return updateFlags;
}

DescriptorSetLayoutBindingResourcesHandler NodeContextDescriptorSetManager::GetCpuDescriptorSetDataImpl(
    const uint32_t index, const vector<CpuDescriptorSet>& cpuDescriptorSet)
{
    if (index < cpuDescriptorSet.size()) {
        return DescriptorSetLayoutBindingResourcesHandler {
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
    const uint32_t index, const vector<CpuDescriptorSet>& cpuDescriptorSet)
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
    const uint32_t index, const vector<CpuDescriptorSet>& cpuDescriptorSet)
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

uint32_t NodeContextDescriptorSetManager::GetDynamicOffsetDescriptorCountImpl(
    const uint32_t index, const vector<CpuDescriptorSet>& cpuDescriptorSet)
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

void NodeContextDescriptorSetManager::IncreaseDescriptorSetCounts(
    const DescriptorSetLayoutBinding& refBinding, LowLevelDescriptorCounts& descSetCounts, uint32_t& dynamicOffsetCount)
{
    if (NodeContextDescriptorSetManager::IsDynamicDescriptor(refBinding.descriptorType)) {
        dynamicOffsetCount++;
    }
    const uint32_t descriptorCount = refBinding.descriptorCount;
    if (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
        descSetCounts.samplerCount += descriptorCount;
    } else if (((refBinding.descriptorType >= CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                   (refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE)) ||
               (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
        descSetCounts.imageCount += descriptorCount;
    } else if (((refBinding.descriptorType >= CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) &&
                   (refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) ||
               (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE)) {
        descSetCounts.bufferCount += descriptorCount;
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!((refBinding.descriptorType <= CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) ||
            (refBinding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE))) {
        PLUGIN_LOG_W("RENDER_VALIDATION: descriptor type not found");
    }
#endif
}

#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
void NodeContextDescriptorSetManager::SetValidationDebugName(const string_view debugName)
{
    debugName_ = debugName;
}
#endif
RENDER_END_NAMESPACE()
