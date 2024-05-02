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

#include "pipeline_descriptor_set_binder.h"

#include <cstdint>

#include <base/math/mathf.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>

#include "device/gpu_resource_handle_util.h"
#include "device/pipeline_state_object.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint8_t INVALID_BIDX { 16 };
constexpr ImageLayout GetImageLayout(const DescriptorType dt)
{
    if ((dt == CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) || (dt == CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
        (dt == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
        return CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } else if (dt == CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
        return CORE_IMAGE_LAYOUT_GENERAL;
    } else {
        return CORE_IMAGE_LAYOUT_UNDEFINED;
    }
}

inline constexpr bool CheckValidBufferDescriptor(const DescriptorType dt)
{
    return ((dt >= CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) && (dt <= CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) ||
           (dt == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE);
}

inline constexpr bool CheckValidImageDescriptor(const DescriptorType dt)
{
    return (dt == CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) || (dt == CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE) ||
           (dt == CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE) || (dt == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
}

struct DescriptorCounts {
    uint32_t count { 0 };
    uint32_t arrayCount { 0 };
};
} // namespace

DescriptorSetBinder::DescriptorSetBinder(
    const RenderHandle handle, const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
    : handle_(handle)
{
    if (RenderHandleUtil::GetHandleType(handle) != RenderHandleType::DESCRIPTOR_SET) {
        PLUGIN_LOG_E("invalid handle in descriptor set binder creation");
    }
    Init(descriptorSetLayoutBindings);
}

DescriptorSetBinder::DescriptorSetBinder(const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    Init(descriptorSetLayoutBindings);
}

void DescriptorSetBinder::Init(const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings)
{
    bindings_.resize(descriptorSetLayoutBindings.size());
    for (size_t idx = 0; idx < descriptorSetLayoutBindings.size(); ++idx) {
        maxBindingCount_ = Math::max(maxBindingCount_, descriptorSetLayoutBindings[idx].binding);
    }
    // +1 for binding count
    maxBindingCount_ = Math::min(maxBindingCount_ + 1, PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT);

#if (RENDER_VALIDATION_ENABLED == 1)
    uint32_t minDescriptorCount = ~0U;
#endif
    DescriptorCounts bufferCounts;
    DescriptorCounts imageCounts;
    DescriptorCounts samplerCounts;
    for (const auto& binding : descriptorSetLayoutBindings) {
        const RenderHandleType type = DescriptorSetBinderUtil::GetRenderHandleType(binding.descriptorType);
        // we don't have duplicates, array descriptor from index 1 can be found directly from arrayOffset
        const uint32_t arrayCount = (binding.descriptorCount > 1) ? (binding.descriptorCount - 1) : 0;
        if (type == RenderHandleType::GPU_BUFFER) {
            bufferCounts.count++;
            bufferCounts.arrayCount += arrayCount;
        } else if (type == RenderHandleType::GPU_IMAGE) {
            imageCounts.count++;
            imageCounts.arrayCount += arrayCount;
        } else if (type == RenderHandleType::GPU_SAMPLER) {
            samplerCounts.count++;
            samplerCounts.arrayCount += arrayCount;
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        minDescriptorCount = Math::min(minDescriptorCount, binding.descriptorCount);
#endif
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if ((minDescriptorCount == 0U) && (!descriptorSetLayoutBindings.empty())) {
        PLUGIN_LOG_W("RENDER_VALIDATION: Descriptors with descriptor count of zero which might indicate non-sized "
                     "arrays in shaders. Resize the descriptor counts before creating descriptor sets and binders.");
    }
#endif
    buffers_.resize(bufferCounts.count + bufferCounts.arrayCount);
    images_.resize(imageCounts.count + imageCounts.arrayCount);
    samplers_.resize(samplerCounts.count + samplerCounts.arrayCount);

    InitFillBindings(descriptorSetLayoutBindings, bufferCounts.count, imageCounts.count, samplerCounts.count);
}

void DescriptorSetBinder::InitFillBindings(
    const array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings, const uint32_t bufferCount,
    const uint32_t imageCount, const uint32_t samplerCount)
{
    auto UpdateArrayStates = [](const auto& desc, auto& vec) {
        const uint32_t maxArrayCount = desc.arrayOffset + desc.binding.descriptorCount - 1;
        for (uint32_t idx = desc.arrayOffset; idx < maxArrayCount; ++idx) {
            vec[idx] = desc;
            vec[idx].binding.descriptorCount = 0;
            vec[idx].arrayOffset = 0;
        }
    };

    DescriptorCounts bufferIdx { 0, bufferCount };
    DescriptorCounts imageIdx { 0, imageCount };
    DescriptorCounts samplerIdx { 0, samplerCount };
    for (size_t idx = 0; idx < bindings_.size(); ++idx) {
        auto& currBinding = bindings_[idx];
        currBinding.binding = descriptorSetLayoutBindings[idx];

        // setup expected bindings
        descriptorBindingMask_ |= (1 << currBinding.binding.binding);

        const AccessFlags accessFlags = DescriptorSetBinderUtil::GetAccessFlags(currBinding.binding.descriptorType);
        const ShaderStageFlags shaderStageFlags = currBinding.binding.shaderStageFlags;
        const PipelineStageFlags pipelineStageFlags =
            DescriptorSetBinderUtil::GetPipelineStageFlags(currBinding.binding.shaderStageFlags);
        if (idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
            bindingToIndex_[currBinding.binding.binding] = static_cast<uint8_t>(idx);
        }
        const RenderHandleType type = DescriptorSetBinderUtil::GetRenderHandleType(currBinding.binding.descriptorType);
        if (type == RenderHandleType::GPU_BUFFER) {
            currBinding.resourceIndex = bufferIdx.count;
            PLUGIN_ASSERT(bufferIdx.count < buffers_.size());
            auto& res = buffers_[bufferIdx.count];
            res.binding = currBinding.binding;
            res.state = { shaderStageFlags, accessFlags, pipelineStageFlags, {} };

            bufferIdx.count++;
            if (res.binding.descriptorCount > 1) {
                res.arrayOffset = bufferIdx.arrayCount;
                bufferIdx.arrayCount += (currBinding.binding.descriptorCount - 1);
                UpdateArrayStates(res, buffers_);
            }
        } else if (type == RenderHandleType::GPU_IMAGE) {
            currBinding.resourceIndex = imageIdx.count;
            PLUGIN_ASSERT(imageIdx.count < images_.size());
            auto& res = images_[imageIdx.count];
            res.binding = currBinding.binding;
            res.state = { shaderStageFlags, accessFlags, pipelineStageFlags, {} };

            imageIdx.count++;
            if (res.binding.descriptorCount > 1) {
                res.arrayOffset = imageIdx.arrayCount;
                imageIdx.arrayCount += (currBinding.binding.descriptorCount - 1);
                UpdateArrayStates(res, images_);
            }
        } else if (type == RenderHandleType::GPU_SAMPLER) {
            currBinding.resourceIndex = samplerIdx.count;
            PLUGIN_ASSERT(samplerIdx.count < samplers_.size());
            auto& res = samplers_[samplerIdx.count];
            res.binding = currBinding.binding;

            samplerIdx.count++;
            if (res.binding.descriptorCount > 1) {
                res.arrayOffset = samplerIdx.arrayCount;
                samplerIdx.arrayCount += (currBinding.binding.descriptorCount - 1);
                UpdateArrayStates(res, samplers_);
            }
        }
    }
}

void DescriptorSetBinder::ClearBindings()
{
    bindingMask_ = 0;
    auto ClearResourcesBindings = [](auto& resources) {
        for (auto& ref : resources) {
            ref.resource = {};
            ref.additionalFlags = 0u;
        }
    };
    ClearResourcesBindings(buffers_);
    ClearResourcesBindings(images_);
    ClearResourcesBindings(samplers_);
}

RenderHandle DescriptorSetBinder::GetDescriptorSetHandle() const
{
    return handle_;
}

DescriptorSetLayoutBindingResources DescriptorSetBinder::GetDescriptorSetLayoutBindingResources() const
{
    return DescriptorSetLayoutBindingResources { bindings_, buffers_, images_, samplers_, descriptorBindingMask_,
        bindingMask_ };
}

void DescriptorSetBinder::PrintDescriptorSetLayoutBindingValidation() const
{
    auto CheckValidity = [](const auto& vec, const string_view strView) {
        for (size_t idx = 0; idx < vec.size(); ++idx) {
            if (!RenderHandleUtil::IsValid(vec[idx].resource.handle)) {
                PLUGIN_LOG_E("RENDER_VALIDATION: invalid resource in descriptor set bindings (binding:%u) (type:%s)",
                    static_cast<uint32_t>(idx), strView.data());
            }
        }
    };
    // NOTE: does not check combined image sampler samplers
    CheckValidity(buffers_, "buffer");
    CheckValidity(images_, "image");
    CheckValidity(samplers_, "sampler");
}

bool DescriptorSetBinder::GetDescriptorSetLayoutBindingValidity() const
{
    return (bindingMask_ == descriptorBindingMask_);
}

void DescriptorSetBinder::BindBuffer(
    const uint32_t binding, const BindableBuffer& resource, const AdditionalDescriptorFlags flags)
{
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(resource.handle);
    const bool validHandleType = (handleType == RenderHandleType::GPU_BUFFER);
    if ((binding < maxBindingCount_) && validHandleType) {
#if (RENDER_VALIDATION_ENABLED == 1)
        if (resource.byteSize == 0) {
            PLUGIN_LOG_ONCE_W("PipelineDescriptorSetBinder::BindBuffer()_byteSize",
                "RENDER_VALIDATION: PipelineDescriptorSetBinder::BindBuffer() byteSize 0");
        }
#endif
        const uint32_t index = bindingToIndex_[binding];
        if (index < INVALID_BIDX) {
            const auto& bind = bindings_[index];
            const DescriptorType descriptorType = bind.binding.descriptorType;
            if (CheckValidBufferDescriptor(descriptorType)) {
                buffers_[bind.resourceIndex].additionalFlags = flags;
                buffers_[bind.resourceIndex].resource = resource;
                bindingMask_ |= (1 << binding);
            } else {
                PLUGIN_LOG_E("invalid binding for buffer descriptor (binding: %u, descriptorType: %u)", binding,
                    (uint32_t)descriptorType);
            }
        }
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!validHandleType) {
        PLUGIN_LOG_ONCE_W("PipelineDescriptorSetBinder::BindBuffer()",
            "RENDER_VALIDATION: PipelineDescriptorSetBinder::BindBuffer() invalid handle");
    }
#endif
}

void DescriptorSetBinder::BindBuffer(const uint32_t binding, const BindableBuffer& resource)
{
    BindBuffer(binding, resource, 0);
}

void DescriptorSetBinder::BindBuffer(const uint32_t binding, const RenderHandle handle, const uint32_t byteOffset)
{
    BindBuffer(binding, BindableBuffer { handle, byteOffset, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE });
}

void DescriptorSetBinder::BindBuffer(
    const uint32_t binding, const RenderHandle handle, const uint32_t byteOffset, const uint32_t byteSize)
{
    BindBuffer(binding, BindableBuffer { handle, byteOffset, byteSize });
}

void DescriptorSetBinder::BindBuffers(const uint32_t binding, const BASE_NS::array_view<const BindableBuffer> resources)
{
    if ((!resources.empty()) && (binding < maxBindingCount_)) {
        const uint32_t index = bindingToIndex_[binding];
        if ((index < INVALID_BIDX) && CheckValidBufferDescriptor(bindings_[index].binding.descriptorType)) {
            const auto& bind = bindings_[index];
            BindableBuffer& ref = buffers_[bind.resourceIndex].resource;
            const uint32_t arrayOffset = buffers_[bind.resourceIndex].arrayOffset;
#if (RENDER_VALIDATION_ENABLED == 1)
            bool validationIssue = false;
#endif
            const uint32_t maxCount = Math::min(static_cast<uint32_t>(resources.size()), bind.binding.descriptorCount);
            PLUGIN_ASSERT((arrayOffset + maxCount - 1) <= buffers_.size());
            for (uint32_t idx = 0; idx < maxCount; ++idx) {
                const RenderHandle currHandle = resources[idx].handle;
                const bool validType = (RenderHandleUtil::GetHandleType(currHandle) == RenderHandleType::GPU_BUFFER);
#if (RENDER_VALIDATION_ENABLED == 1)
                if (!validType) {
                    PLUGIN_LOG_E(
                        "RENDER_VALIDATION: PipelineDescriptorSetBinder::BindBuffers() invalid handle type (index: "
                        "%u)",
                        static_cast<uint32_t>(idx));
                }
                if (bind.binding.descriptorCount != resources.size()) {
                    validationIssue = true;
                }
#endif
                if (validType) {
                    BindableBuffer& bRes = (idx == 0) ? ref : buffers_[arrayOffset + idx - 1].resource;
                    bRes = resources[idx];
                    bindingMask_ |= (1 << binding);
                }
            }
#if (RENDER_VALIDATION_ENABLED == 1)
            if (validationIssue) {
                PLUGIN_LOG_E(
                    "RENDER_VALIDATION: DescriptorSetBinder::BindBuffers() trying to bind (%u) buffers to set arrays "
                    "(%u)",
                    static_cast<uint32_t>(resources.size()), bind.binding.descriptorCount);
            }
#endif
        } else {
            PLUGIN_LOG_E("invalid binding for buffer descriptor (binding: %u)", binding);
        }
    }
}

void DescriptorSetBinder::BindImage(
    const uint32_t binding, const BindableImage& resource, const AdditionalDescriptorFlags flags)
{
    const bool validHandleType = (RenderHandleUtil::GetHandleType(resource.handle) == RenderHandleType::GPU_IMAGE);
    if ((binding < maxBindingCount_) && validHandleType) {
        const uint32_t index = bindingToIndex_[binding];
        if (index < INVALID_BIDX) {
            const auto& bind = bindings_[index];
            const DescriptorType descriptorType = bind.binding.descriptorType;
            const bool validSamplerHandling =
                (descriptorType == CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                    ? (RenderHandleUtil::GetHandleType(resource.samplerHandle) == RenderHandleType::GPU_SAMPLER)
                    : true;
            if (CheckValidImageDescriptor(descriptorType) && validSamplerHandling) {
                images_[bind.resourceIndex].additionalFlags = flags;
                BindableImage& bindableImage = images_[bind.resourceIndex].resource;
                if (resource.imageLayout != CORE_IMAGE_LAYOUT_UNDEFINED) {
                    bindableImage.imageLayout = resource.imageLayout;
                } else {
                    bindableImage.imageLayout = (RenderHandleUtil::IsDepthImage(resource.handle))
                                                    ? ImageLayout::CORE_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                                    : GetImageLayout(bind.binding.descriptorType);
                }
                bindableImage.handle = resource.handle;
                bindableImage.mip = resource.mip;
                bindableImage.layer = resource.layer;
                bindableImage.samplerHandle = resource.samplerHandle;
                bindingMask_ |= (1 << binding);
            } else {
                PLUGIN_LOG_E("invalid binding for image descriptor (binding: %u, descriptorType: %u)", binding,
                    (uint32_t)descriptorType);
            }
        }
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!validHandleType) {
        PLUGIN_LOG_ONCE_W("PipelineDescriptorSetBinder::BindImage()",
            "RENDER_VALIDATION: PipelineDescriptorSetBinder::BindImage() invalid handle");
    }
#endif
}

void DescriptorSetBinder::BindImage(const uint32_t binding, const BindableImage& resource)
{
    BindImage(binding, resource, 0);
}

void DescriptorSetBinder::BindImage(const uint32_t binding, const RenderHandle handle)
{
    BindImage(
        binding, BindableImage { handle, PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS,
                     PipelineStateConstants::GPU_IMAGE_ALL_LAYERS, ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED, {} });
}

void DescriptorSetBinder::BindImage(const uint32_t binding, const RenderHandle handle, const RenderHandle samplerHandle)
{
    BindImage(binding,
        BindableImage { handle, PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS,
            PipelineStateConstants::GPU_IMAGE_ALL_LAYERS, ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED, samplerHandle });
}

void DescriptorSetBinder::BindImages(const uint32_t binding, const array_view<const BindableImage> resources)
{
    if ((!resources.empty()) && (binding < maxBindingCount_)) {
        const uint32_t index = bindingToIndex_[binding];
        if ((index < INVALID_BIDX) && CheckValidImageDescriptor(bindings_[index].binding.descriptorType)) {
            const auto& bind = bindings_[index];
            BindableImage& ref = images_[bind.resourceIndex].resource;
            const uint32_t arrayOffset = images_[bind.resourceIndex].arrayOffset;
            const ImageLayout defaultImageLayout = GetImageLayout(bind.binding.descriptorType);
            const uint32_t maxCount = Math::min(static_cast<uint32_t>(resources.size()), bind.binding.descriptorCount);
            PLUGIN_ASSERT((arrayOffset + maxCount - 1) <= images_.size());
            for (uint32_t idx = 0; idx < maxCount; ++idx) {
                const BindableImage& currResource = resources[idx];
                const RenderHandle currHandle = currResource.handle;
                const bool validType = (RenderHandleUtil::GetHandleType(currHandle) == RenderHandleType::GPU_IMAGE);
#if (RENDER_VALIDATION_ENABLED == 1)
                if (!validType) {
                    PLUGIN_LOG_E("RENDER_VALIDATION: DescriptorSetBinder::BindImages() invalid handle type (idx:%u)",
                        static_cast<uint32_t>(idx));
                }
#endif
                if (validType) {
                    BindableImage& bindableImage = (idx == 0) ? ref : images_[arrayOffset + idx - 1].resource;
                    if (currResource.imageLayout != CORE_IMAGE_LAYOUT_UNDEFINED) {
                        bindableImage.imageLayout = currResource.imageLayout;
                    } else {
                        bindableImage.imageLayout = (RenderHandleUtil::IsDepthImage(currResource.handle))
                                                        ? ImageLayout::CORE_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                                        : defaultImageLayout;
                    }
                    bindableImage.handle = currResource.handle;
                    bindableImage.mip = currResource.mip;
                    bindableImage.layer = currResource.layer;
                    bindableImage.samplerHandle = currResource.samplerHandle;
                    bindingMask_ |= (1 << binding);
                }
            }
#if (RENDER_VALIDATION_ENABLED == 1)
            if ((bind.binding.descriptorCount != resources.size())) {
                PLUGIN_LOG_E(
                    "RENDER_VALIDATION: DescriptorSetBinder::BindImages() trying binding (%u) images to arrays (%u)",
                    static_cast<uint32_t>(resources.size()), bind.binding.descriptorCount);
            }
#endif
        }
    }
}

void DescriptorSetBinder::BindSampler(
    const uint32_t binding, const BindableSampler& resource, const AdditionalDescriptorFlags flags)
{
    const bool validHandleType = (RenderHandleUtil::GetHandleType(resource.handle) == RenderHandleType::GPU_SAMPLER);
    if ((binding < maxBindingCount_) && validHandleType) {
        const uint32_t index = bindingToIndex_[binding];
        if (index < INVALID_BIDX) {
            const auto& bind = bindings_[index];
            const DescriptorType descriptorType = bind.binding.descriptorType;
            if (descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
                samplers_[bind.resourceIndex].additionalFlags = flags;
                samplers_[bind.resourceIndex].resource = resource;
                bindingMask_ |= (1 << binding);
            } else {
                PLUGIN_LOG_E("invalid binding for sampler descriptor (binding: %u, descriptorType: %u)", binding,
                    (uint32_t)descriptorType);
            }
        }
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!validHandleType) {
        PLUGIN_LOG_ONCE_W("PipelineDescriptorSetBinder::BindSampler()",
            "RENDER_VALIDATION: PipelineDescriptorSetBinder::BindSampler() invalid handle");
    }
#endif
}

void DescriptorSetBinder::BindSampler(const uint32_t binding, const BindableSampler& resource)
{
    BindSampler(binding, resource, 0);
}

void DescriptorSetBinder::BindSampler(const uint32_t binding, const RenderHandle handle)
{
    BindSampler(binding, BindableSampler { handle });
}

void DescriptorSetBinder::BindSamplers(const uint32_t binding, const array_view<const BindableSampler> resources)
{
    if ((!resources.empty()) && (binding < maxBindingCount_)) {
        const uint32_t index = bindingToIndex_[binding];
        if ((index < INVALID_BIDX) && (bindings_[index].binding.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER)) {
            const auto& bind = bindings_[index];
            BindableSampler& ref = samplers_[bind.resourceIndex].resource;
            const uint32_t arrayOffset = samplers_[bind.resourceIndex].arrayOffset;
#if (RENDER_VALIDATION_ENABLED == 1)
            bool validationIssue = false;
#endif
            const uint32_t maxCount = Math::min(static_cast<uint32_t>(resources.size()), bind.binding.descriptorCount);
            PLUGIN_ASSERT((arrayOffset + maxCount - 1) <= samplers_.size());
            for (uint32_t idx = 0; idx < maxCount; ++idx) {
                const RenderHandle currHandle = resources[idx].handle;
                const bool validType = (RenderHandleUtil::GetHandleType(currHandle) == RenderHandleType::GPU_SAMPLER);
#if (RENDER_VALIDATION_ENABLED == 1)
                if (!validType) {
                    PLUGIN_LOG_E(
                        "RENDER_VALIDATION: PipelineDescriptorSetBinder::BindSamplers() invalid handle type (index: "
                        "%u)",
                        static_cast<uint32_t>(idx));
                }
                if (bind.binding.descriptorCount != resources.size()) {
                    validationIssue = true;
                }
#endif
                if (validType) {
                    BindableSampler& bRes = (idx == 0) ? ref : samplers_[arrayOffset + idx - 1].resource;
                    bRes = resources[idx];
                    bindingMask_ |= (1 << binding);
                }
            }
#if (RENDER_VALIDATION_ENABLED == 1)
            if (validationIssue) {
                PLUGIN_LOG_E(
                    "RENDER_VALIDATION: DescriptorSetBinder::BindSamplers() trying to bind (%u) samplers to set "
                    "arrays (%u)",
                    static_cast<uint32_t>(resources.size()), bind.binding.descriptorCount);
            }
#endif
        }
    }
}

void DescriptorSetBinder::Destroy()
{
    delete this;
}

PipelineDescriptorSetBinder::PipelineDescriptorSetBinder(const PipelineLayout& pipelineLayout,
    const array_view<const RenderHandle> handles,
    const array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings)
{
    Init(pipelineLayout, handles, descriptorSetsLayoutBindings, true);
}

PipelineDescriptorSetBinder::PipelineDescriptorSetBinder(const PipelineLayout& pipelineLayout,
    const array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings)
{
    PLUGIN_STATIC_ASSERT(PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT == 4U);
    RenderHandle handles[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] { {}, {}, {}, {} };
    Init(pipelineLayout, { handles, descriptorSetsLayoutBindings.size() }, descriptorSetsLayoutBindings, false);
}

void PipelineDescriptorSetBinder::Init(const PipelineLayout& pipelineLayout,
    const BASE_NS::array_view<const RenderHandle> handles,
    const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings, bool validHandles)
{
    descriptorSetBinders_.reserve(descriptorSetsLayoutBindings.size());
    setIndices_.reserve(descriptorSetsLayoutBindings.size());
    descriptorSetHandles_.reserve(descriptorSetsLayoutBindings.size());

    uint32_t setToBinderIdx = 0;
    for (uint32_t setIdx = 0; setIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++setIdx) {
        const auto& pipelineDescRef = pipelineLayout.descriptorSetLayouts[setIdx];
        if (pipelineDescRef.set >= PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
            continue;
        }

        const uint32_t set = pipelineDescRef.set;
        const auto& descRef = descriptorSetsLayoutBindings[setIdx];
        const auto descHandle = handles[setIdx];

        // create binder
        if (validHandles) {
            descriptorSetBinders_.emplace_back(descHandle, descRef.binding);
        } else {
            descriptorSetBinders_.emplace_back(descRef.binding);
        }
        setIndices_.push_back(set);
        descriptorSetHandles_.push_back(descHandle);

        vector<DescriptorSetLayoutBinding> dslb(pipelineDescRef.bindings.size());
        for (size_t bindingIdx = 0; bindingIdx < pipelineDescRef.bindings.size(); ++bindingIdx) {
            const auto& descPipelineRef = descRef.binding[bindingIdx];
#if (RENDER_VALIDATION_ENABLED == 1)
            if (pipelineDescRef.bindings[bindingIdx].binding != descPipelineRef.binding) {
                PLUGIN_LOG_E(
                    "RENDER_VALIDATION: pipeline layout binding does not match descriptor set layout binding or "
                    "handle");
            }
#endif

            dslb[bindingIdx] = descPipelineRef;
        }
        setToBinderIndex_[set] = setToBinderIdx++;
    }
}

void PipelineDescriptorSetBinder::ClearBindings()
{
    for (auto& ref : descriptorSetBinders_) {
        ref.ClearBindings();
    }
}

void PipelineDescriptorSetBinder::BindBuffer(
    const uint32_t set, const uint32_t binding, const BindableBuffer& resource, const AdditionalDescriptorFlags flags)
{
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetBinders_.size())) {
            descriptorSetBinders_[setIdx].BindBuffer(binding, resource, flags);
        }
    }
}

void PipelineDescriptorSetBinder::BindBuffer(const uint32_t set, const uint32_t binding, const BindableBuffer& resource)
{
    BindBuffer(set, binding, resource, 0);
}

void PipelineDescriptorSetBinder::BindBuffers(
    const uint32_t set, const uint32_t binding, const array_view<const BindableBuffer> resources)
{
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetBinders_.size())) {
            descriptorSetBinders_[setIdx].BindBuffers(binding, resources);
        }
    }
}

void PipelineDescriptorSetBinder::BindImage(
    const uint32_t set, const uint32_t binding, const BindableImage& resource, const AdditionalDescriptorFlags flags)
{
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetBinders_.size())) {
            descriptorSetBinders_[setIdx].BindImage(binding, resource, flags);
        }
    }
}

void PipelineDescriptorSetBinder::BindImage(const uint32_t set, const uint32_t binding, const BindableImage& resource)
{
    BindImage(set, binding, resource, 0);
}

void PipelineDescriptorSetBinder::BindImages(
    const uint32_t set, const uint32_t binding, const array_view<const BindableImage> resources)
{
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetBinders_.size())) {
            descriptorSetBinders_[setIdx].BindImages(binding, resources);
        }
    }
}

void PipelineDescriptorSetBinder::BindSampler(
    const uint32_t set, const uint32_t binding, const BindableSampler& resource, const AdditionalDescriptorFlags flags)
{
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetBinders_.size())) {
            descriptorSetBinders_[setIdx].BindSampler(binding, resource, flags);
        }
    }
}

void PipelineDescriptorSetBinder::BindSampler(
    const uint32_t set, const uint32_t binding, const BindableSampler& resource)
{
    BindSampler(set, binding, resource, 0);
}

void PipelineDescriptorSetBinder::BindSamplers(
    const uint32_t set, const uint32_t binding, const array_view<const BindableSampler> resources)
{
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetBinders_.size())) {
            descriptorSetBinders_[setIdx].BindSamplers(binding, resources);
        }
    }
}

RenderHandle PipelineDescriptorSetBinder::GetDescriptorSetHandle(const uint32_t set) const
{
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetBinders_.size())) {
            return descriptorSetBinders_[setIdx].GetDescriptorSetHandle();
        }
    }
    return {};
}

DescriptorSetLayoutBindingResources PipelineDescriptorSetBinder::GetDescriptorSetLayoutBindingResources(
    const uint32_t set) const
{
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetBinders_.size())) {
            return descriptorSetBinders_[setIdx].GetDescriptorSetLayoutBindingResources();
        }
    }
    return {};
}

bool PipelineDescriptorSetBinder::GetPipelineDescriptorSetLayoutBindingValidity() const
{
    bool valid = true;
    for (const auto& ref : descriptorSetBinders_) {
        if (!ref.GetDescriptorSetLayoutBindingValidity()) {
            valid = false;
        }
    }
    return valid;
}

void PipelineDescriptorSetBinder::PrintPipelineDescriptorSetLayoutBindingValidation() const
{
    for (const auto& ref : descriptorSetBinders_) {
        ref.PrintDescriptorSetLayoutBindingValidation();
    }
}

uint32_t PipelineDescriptorSetBinder::GetDescriptorSetCount() const
{
    return (uint32_t)descriptorSetBinders_.size();
}

array_view<const uint32_t> PipelineDescriptorSetBinder::GetSetIndices() const
{
    return array_view<const uint32_t>(setIndices_.data(), setIndices_.size());
}

array_view<const RenderHandle> PipelineDescriptorSetBinder::GetDescriptorSetHandles() const
{
    return { descriptorSetHandles_.data(), descriptorSetHandles_.size() };
}

uint32_t PipelineDescriptorSetBinder::GetFirstSet() const
{
    uint32_t set = 0;
    if (!setIndices_.empty()) {
        set = setIndices_[0];
    }
    return set;
}

array_view<const RenderHandle> PipelineDescriptorSetBinder::GetDescriptorSetHandles(
    const uint32_t beginSet, const uint32_t count) const
{
    if (beginSet < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[beginSet];
        if (setIdx < static_cast<uint32_t>(descriptorSetBinders_.size())) {
            const uint32_t maxCount = Math::min(count, static_cast<uint32_t>(descriptorSetHandles_.size()) - beginSet);
#if (RENDER_VALIDATION_ENABLED == 1)
            if (setIdx + count > descriptorSetHandles_.size()) {
                PLUGIN_LOG_ONCE_W("PipelineDescriptorSetBinder::GetDescriptorSetHandles()",
                    "RENDER_VALIDATION: PipelineDescriptorSetBinder::GetDescriptorSetHandles() count exceeds sets");
            }
#endif

            return array_view<const RenderHandle>(&descriptorSetHandles_[setIdx], maxCount);
        }
    }
    return {};
}

void PipelineDescriptorSetBinder::Destroy()
{
    delete this;
}
RENDER_END_NAMESPACE()
