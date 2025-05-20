/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "node_context_descriptor_set_manager_gles.h"

#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_resource_manager.h"
#include "gles/gl_functions.h"
#include "gles/gpu_buffer_gles.h"
#include "gles/gpu_image_gles.h"
#include "gles/gpu_sampler_gles.h"
#include "nodecontext/pipeline_descriptor_set_binder.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
CpuDescriptorSet CreateCpuDescriptorSetData(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    uint32_t dynamicOffsetCount = 0;
    CpuDescriptorSet newSet;
    newSet.bindings.reserve(descriptorSetLayoutBindings.size());
    LowLevelDescriptorCounts descriptorCounts;
    for (const auto& refBinding : descriptorSetLayoutBindings) {
        // NOTE: sort from 0 to n
        newSet.bindings.push_back({ refBinding, {} });
        NodeContextDescriptorSetManager::IncreaseDescriptorSetCounts(refBinding, descriptorCounts, dynamicOffsetCount);
    }
    newSet.buffers.resize(descriptorCounts.bufferCount);
    newSet.images.resize(descriptorCounts.imageCount);
    newSet.samplers.resize(descriptorCounts.samplerCount);

    newSet.dynamicOffsetDescriptors.resize(dynamicOffsetCount);
    return newSet;
}

constexpr uint32_t GetArrayOffset(const CpuDescriptorSet& data, const DescriptorSetLayoutBindingResource& res)
{
    const RenderHandleType type = DescriptorSetBinderUtil::GetRenderHandleType(res.binding.descriptorType);
    if (type == RenderHandleType::GPU_BUFFER) {
        return data.buffers[res.resourceIndex].desc.arrayOffset;
    }
    if (type == RenderHandleType::GPU_IMAGE) {
        return data.images[res.resourceIndex].desc.arrayOffset;
    }
    if (type == RenderHandleType::GPU_SAMPLER) {
        return data.samplers[res.resourceIndex].desc.arrayOffset;
    }
    return 0u;
}

inline void BindSampler(GpuResourceManager& gpuResourceMgr, const BindableSampler& res, Gles::Bind& obj, uint32_t index)
{
    if (const auto* gpuSampler = gpuResourceMgr.GetSampler<GpuSamplerGLES>(res.handle)) {
        const auto& plat = gpuSampler->GetPlatformData();
        obj.resources[index].sampler.samplerId = plat.sampler;
    } else {
        obj.resources[index].sampler.samplerId = 0;
    }
}

void BindImage(GpuResourceManager& gpuResourceMgr_, const BindableImage& res, const GpuResourceState& resState,
    Gles::Bind& obj, uint32_t index)
{
    const AccessFlags accessFlags = resState.accessFlags;
    auto* gpuImage = gpuResourceMgr_.GetImage<GpuImageGLES>(res.handle);
    auto& ref = obj.resources[index];
    ref.image.image = gpuImage;
    const bool read = (accessFlags & CORE_ACCESS_SHADER_READ_BIT) == CORE_ACCESS_SHADER_READ_BIT;
    const bool write = (accessFlags & CORE_ACCESS_SHADER_WRITE_BIT) == CORE_ACCESS_SHADER_WRITE_BIT;
    if (read && write) {
        ref.image.mode = GL_READ_WRITE;
    } else if (read) {
        ref.image.mode = GL_READ_ONLY;
    } else if (write) {
        ref.image.mode = GL_WRITE_ONLY;
    } else {
        // no read and no write?
        ref.image.mode = GL_READ_WRITE;
    }
#if RENDER_HAS_GLES_BACKEND
    if ((gpuImage) && (gpuImage->GetPlatformData().type == GL_TEXTURE_EXTERNAL_OES)) {
        ref.image.mode |= Gles::EXTERNAL_BIT;
    }
#endif
    ref.image.mipLevel = res.mip;
}

inline void BindImageSampler(GpuResourceManager& gpuResourceMgr, const BindableImage& res,
    const GpuResourceState& resState, Gles::Bind& obj, uint32_t index)
{
    BindImage(gpuResourceMgr, res, resState, obj, index);
    BindSampler(gpuResourceMgr, BindableSampler { res.samplerHandle }, obj, index);
}

void BindBuffer(GpuResourceManager& gpuResourceMgr, const BindableBuffer& res, Gles::Bind& obj, uint32_t index)
{
    if (const auto* gpuBuffer = gpuResourceMgr.GetBuffer<GpuBufferGLES>(res.handle)) {
        const auto& plat = gpuBuffer->GetPlatformData();
        const uint32_t baseOffset = res.byteOffset;
        obj.resources[index].buffer.offset = baseOffset + plat.currentByteOffset;
        obj.resources[index].buffer.size = Math::min(plat.bindMemoryByteSize - baseOffset, res.byteSize);
        obj.resources[index].buffer.bufferId = plat.buffer;
    } else {
        obj.resources[index].buffer = {};
    }
}

void ConvertDescSetToResource(GpuResourceManager& gpuResMgr, const CpuDescriptorSet& srcSet, array_view<Gles::Bind> dst)
{
    for (const auto& bindings : srcSet.bindings) {
        auto& obj = dst[bindings.binding.binding];
        const auto descriptorType = bindings.binding.descriptorType;
        obj.descriptorType = descriptorType;
        obj.resources.resize(bindings.binding.descriptorCount);

        const bool hasArrOffset = (bindings.binding.descriptorCount > 1U);
        const uint32_t arrayOffset = hasArrOffset ? GetArrayOffset(srcSet, bindings) : 0;
        for (uint32_t index = 0; index < bindings.binding.descriptorCount; index++) {
            const uint32_t resIdx = (index == 0) ? bindings.resourceIndex : (arrayOffset + index - 1);
            switch (descriptorType) {
                case CORE_DESCRIPTOR_TYPE_SAMPLER: {
                    const auto& samplerDescriptor = srcSet.samplers[resIdx].desc;
                    BindSampler(gpuResMgr, samplerDescriptor.resource, obj, index);
                    break;
                }
                case CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                case CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
                    const auto& imageDescriptor = srcSet.images[resIdx].desc;
                    BindImage(gpuResMgr, imageDescriptor.resource, imageDescriptor.state, obj, index);
                    break;
                }
                case CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
                    const auto& imageDescriptor = srcSet.images[resIdx].desc;
                    BindImageSampler(gpuResMgr, imageDescriptor.resource, imageDescriptor.state, obj, index);
                    break;
                }
                case CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
                    const auto& bufferDescriptor = srcSet.buffers[resIdx].desc;
                    BindBuffer(gpuResMgr, bufferDescriptor.resource, obj, index);
                    break;
                }
                case CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                    const auto& bufferDescriptor = srcSet.buffers[resIdx].desc;
                    BindBuffer(gpuResMgr, bufferDescriptor.resource, obj, index);
                    break;
                }
                case CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                case CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                case CORE_DESCRIPTOR_TYPE_MAX_ENUM:
                default:
                    PLUGIN_ASSERT_MSG(false, "Unhandled descriptor type");
                    break;
            }
        }
    }
}
} // namespace

DescriptorSetManagerGles::DescriptorSetManagerGles(Device& device) : DescriptorSetManager(device) {}

void DescriptorSetManagerGles::BeginFrame()
{
    DescriptorSetManager::BeginFrame();
}

void DescriptorSetManagerGles::BeginBackendFrame()
{
    // handle write locking
    // handle possible destruction
    resources_.resize(descriptorSets_.size());
    auto resourcesIt = resources_.begin();
    for (const auto& descriptorSet : descriptorSets_) {
        if (GlobalDescriptorSetBase* descriptorSetBase = descriptorSet.get(); descriptorSetBase) {
            bool destroyDescriptorSets = true;
            // if we have any descriptor sets in use we do not destroy the pool
            for (auto& ref : descriptorSetBase->data) {
                if (ref.renderHandleReference.GetRefCount() > 1) {
                    destroyDescriptorSets = false;
                }
                ref.frameWriteLocked = false;
            }

            if (destroyDescriptorSets) {
                if (!descriptorSetBase->data.empty()) {
                    const RenderHandle handle = descriptorSetBase->data[0U].renderHandleReference.GetHandle();
                    // set handle (index location) to be available
                    availableHandles_.push_back(handle);
                }
                nameToIndex_.erase(descriptorSetBase->name);
                *descriptorSetBase = {};
                resourcesIt->clear();
            } else {
                resourcesIt->resize(descriptorSet->data.size());
            }
        }
        ++resourcesIt;
    }
}

void DescriptorSetManagerGles::CreateDescriptorSets(const uint32_t arrayIndex, const uint32_t descriptorSetCount,
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    PLUGIN_ASSERT((arrayIndex < descriptorSets_.size()) && (descriptorSets_[arrayIndex]));
    PLUGIN_ASSERT(descriptorSets_[arrayIndex]->data.size() == descriptorSetCount);
    if ((arrayIndex < descriptorSets_.size()) && (descriptorSets_[arrayIndex])) {
        resources_.resize(descriptorSets_.size());
        resources_[arrayIndex].resize(descriptorSetCount);
        GlobalDescriptorSetBase* cpuData = descriptorSets_[arrayIndex].get();
        for (uint32_t idx = 0; idx < descriptorSetCount; ++idx) {
            const uint32_t additionalIndex = idx;
            cpuData->data[additionalIndex].cpuDescriptorSet = CreateCpuDescriptorSetData(descriptorSetLayoutBindings);
        }
    }
}

bool DescriptorSetManagerGles::UpdateDescriptorSetGpuHandle(const RenderHandle& handle)
{
    // vulkan runs the inherited method, gles does not need it
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t additionalIndex = RenderHandleUtil::GetAdditionalIndexPart(handle);
#if (RENDER_VALIDATION_ENABLED == 1)
    PLUGIN_ASSERT(
        RenderHandleUtil::GetAdditionalData(handle) == NodeContextDescriptorSetManager::GLOBAL_DESCRIPTOR_BIT);

    if (arrayIndex >= static_cast<uint32_t>(descriptorSets_.size())) {
        PLUGIN_LOG_E("invalid handle in descriptor set management");
    }
#endif
    auto& gpuResMgr = static_cast<GpuResourceManager&>(device_.GetGpuResourceManager());
    if ((arrayIndex >= descriptorSets_.size()) || (!descriptorSets_[arrayIndex]) ||
        (additionalIndex >= descriptorSets_[arrayIndex]->data.size())) {
        return false;
    }
    const auto& srcSet = descriptorSets_[arrayIndex]->data[additionalIndex].cpuDescriptorSet;
    auto& dst = resources_[arrayIndex][additionalIndex];
    dst.resize(srcSet.bindings.size());
    ConvertDescSetToResource(gpuResMgr, srcSet, dst);

    return true;
}

void DescriptorSetManagerGles::UpdateCpuDescriptorSetPlatform(
    const DescriptorSetLayoutBindingResources& bindingResources)
{}

array_view<const Gles::Bind> DescriptorSetManagerGles::GetResources(RenderHandle handle) const
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t additionalIndex = RenderHandleUtil::GetAdditionalIndexPart(handle);
    if (arrayIndex < resources_.size()) {
        const auto& set = resources_[arrayIndex];
        if (additionalIndex < set.size()) {
            return set[additionalIndex];
        }
    }
    return {};
}

NodeContextDescriptorSetManagerGles::NodeContextDescriptorSetManagerGles(Device& device)
    : NodeContextDescriptorSetManager(device), device_ { device }
{
    PLUGIN_UNUSED(device_);
}

NodeContextDescriptorSetManagerGles::~NodeContextDescriptorSetManagerGles() = default;

void NodeContextDescriptorSetManagerGles::ResetAndReserve(const DescriptorCounts& descriptorCounts)
{
    NodeContextDescriptorSetManager::ResetAndReserve(descriptorCounts);
}

void NodeContextDescriptorSetManagerGles::BeginFrame()
{
    NodeContextDescriptorSetManager::BeginFrame();

#if (RENDER_VALIDATION_ENABLED == 1)
    oneFrameDescSetGeneration_ = (oneFrameDescSetGeneration_ + 1) % MAX_ONE_FRAME_GENERATION_IDX;
#endif
}

RenderHandle NodeContextDescriptorSetManagerGles::CreateDescriptorSet(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    RenderHandle clientHandle;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_STATIC];
#if (RENDER_VALIDATION_ENABLED == 1)
    if (cpuDescriptorSets.size() >= maxSets_) {
        PLUGIN_LOG_E("RENDER_VALIDATION: No more descriptor sets available");
    }
#endif
    if (cpuDescriptorSets.size() < maxSets_) {
        const auto arrayIndex = (uint32_t)cpuDescriptorSets.size();
        cpuDescriptorSets.push_back(CreateCpuDescriptorSetData(descriptorSetLayoutBindings));
        auto& resources = resources_[DESCRIPTOR_SET_INDEX_TYPE_STATIC].emplace_back();
        resources.resize(descriptorSetLayoutBindings.size());
        // NOTE: can be used directly to index
        clientHandle = RenderHandleUtil::CreateHandle(RenderHandleType::DESCRIPTOR_SET, arrayIndex, 0);
    }
    return clientHandle;
}

RenderHandle NodeContextDescriptorSetManagerGles::CreateOneFrameDescriptorSet(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    RenderHandle clientHandle;
    auto& cpuDescriptorSets = cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME];

    const auto arrayIndex = static_cast<uint32_t>(cpuDescriptorSets.size());
    cpuDescriptorSets.push_back(CreateCpuDescriptorSetData(descriptorSetLayoutBindings));
    auto& resources = resources_[DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME].emplace_back();
    resources.resize(descriptorSetLayoutBindings.size());
    //  NOTE: can be used directly to index
    clientHandle = RenderHandleUtil::CreateHandle(
        RenderHandleType::DESCRIPTOR_SET, arrayIndex, oneFrameDescSetGeneration_, ONE_FRAME_DESC_SET_BIT);

    return clientHandle;
}

bool NodeContextDescriptorSetManagerGles::UpdateDescriptorSetGpuHandle(const RenderHandle handle)
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit & ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                           : DESCRIPTOR_SET_INDEX_TYPE_STATIC;
    auto& cpuDescriptorSets = cpuDescriptorSets_[descSetIdx];
    auto& resources = resources_[descSetIdx];
#if (RENDER_VALIDATION_ENABLED == 1)
    if (arrayIndex >= static_cast<uint32_t>(cpuDescriptorSets.size())) {
        PLUGIN_LOG_E("invalid handle in descriptor set management");
    }
    if (oneFrameDescBit & ONE_FRAME_DESC_SET_BIT) {
        const uint32_t generationIndex = RenderHandleUtil::GetGenerationIndexPart(handle);
        if (generationIndex != oneFrameDescSetGeneration_) {
            PLUGIN_LOG_E(
                "RENDER_VALIDATION: invalid one frame descriptor set handle generation. One frame descriptor sets can "
                "only be used once.");
        }
    }
#endif
    if (cpuDescriptorSets.size() > resources.size()) {
        // This shouldn't happen unless CreateDescriptorSet hasn't been called.
        resources.resize(cpuDescriptorSets.size());
    }
    const auto& srcSet = cpuDescriptorSets[arrayIndex];
    auto& dst = resources[arrayIndex];
    if (srcSet.bindings.size() > dst.size()) {
        // This shouldn't happen unless CreateDescriptorSet hasn't been called.
        dst.resize(srcSet.bindings.size());
    }

    auto& gpuResMgr = static_cast<GpuResourceManager&>(device_.GetGpuResourceManager());
    ConvertDescSetToResource(gpuResMgr, srcSet, dst);

    return true;
}

void NodeContextDescriptorSetManagerGles::UpdateCpuDescriptorSetPlatform(
    const DescriptorSetLayoutBindingResources& bindingResources)
{
    // no op
}

array_view<const Gles::Bind> NodeContextDescriptorSetManagerGles::GetResources(RenderHandle handle) const
{
    const uint32_t additionalData = RenderHandleUtil::GetAdditionalData(handle);
    if (additionalData & NodeContextDescriptorSetManager::GLOBAL_DESCRIPTOR_BIT) {
        return static_cast<DescriptorSetManagerGles&>(globalDescriptorSetMgr_).GetResources(handle);
    }

    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    const uint32_t oneFrameDescBit = RenderHandleUtil::GetAdditionalData(handle);
    const uint32_t descSetIdx = (oneFrameDescBit & ONE_FRAME_DESC_SET_BIT) ? DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME
                                                                           : DESCRIPTOR_SET_INDEX_TYPE_STATIC;

    const auto& resources = resources_[descSetIdx];
    if (arrayIndex < resources.size()) {
        return resources[arrayIndex];
    }
    return {};
}
RENDER_END_NAMESPACE()
