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

#include "shader_pipeline_binder.h"

#include <algorithm>
#include <atomic>

#include <base/containers/allocator.h>
#include <base/containers/array_view.h>
#include <base/math/matrix_util.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
void ShaderPipelineBinder::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void ShaderPipelineBinder::Unref()
{
    if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

ShaderPipelineBinder::ShaderPipelineBinder(const RenderHandleReference& shader, const PipelineLayout& pipelineLayout)
    : shader_(shader), pipelineLayout_(pipelineLayout)
{
    for (uint32_t idx = 0; idx < pipelineLayout.descriptorSetCount; ++idx) {
        const uint32_t set = pipelineLayout.descriptorSetLayouts[idx].set;
        if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
            descriptorSetResources_.emplace_back();
            auto& resSet = descriptorSetResources_.back();
            resSet.bindings.resize(pipelineLayout.descriptorSetLayouts[idx].bindings.size());
            uint32_t bufferCount = 0;
            uint32_t imageCount = 0;
            uint32_t samplerCount = 0;
            for (uint32_t bindingIdx = 0; bindingIdx < pipelineLayout.descriptorSetLayouts[idx].bindings.size();
                 ++bindingIdx) {
                const auto& binding = pipelineLayout.descriptorSetLayouts[idx].bindings[bindingIdx];
                auto& resBinding = resSet.bindings[bindingIdx];
                resBinding.binding = binding.binding;
                resSet.bindingToIndex[resBinding.binding] = static_cast<uint8_t>(bindingIdx);
                resSet.maxBindingCount = Math::max(resSet.maxBindingCount, resBinding.binding);

                if (binding.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
                    resBinding.resIdx = samplerCount;
                    resBinding.type = RenderHandleType::GPU_SAMPLER;
                    samplerCount += binding.descriptorCount;
                } else if (((binding.descriptorType >= CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) &&
                               (binding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE)) ||
                           (binding.descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
                    resBinding.resIdx = imageCount;
                    resBinding.type = RenderHandleType::GPU_IMAGE;
                    imageCount += binding.descriptorCount;
                } else if (((binding.descriptorType >= CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) &&
                               (binding.descriptorType <= CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) ||
                           (binding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE)) {
                    resBinding.resIdx = bufferCount;
                    resBinding.type = RenderHandleType::GPU_BUFFER;
                    bufferCount += binding.descriptorCount;
                }
            }
            // +1 for binding count
            resSet.maxBindingCount =
                Math::min(resSet.maxBindingCount + 1, PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT);

            resSet.buffers.resize(bufferCount);
            resSet.images.resize(imageCount);
            resSet.samplers.resize(samplerCount);
        }
    }
    if (pipelineLayout_.pushConstant.byteSize > 0) {
        pushData_.resize(pipelineLayout_.pushConstant.byteSize);
        std::fill(pushData_.begin(), pushData_.end(), uint8_t(0));
    }
}

bool ShaderPipelineBinder::GetBindingValidity() const
{
    bool valid = true;
    auto checkValidity = [](const auto& vec, bool& valid) {
        for (const auto ref : vec) {
            if (!ref.handle) {
                valid = false;
            }
        }
    };
    for (const auto& ref : descriptorSetResources_) {
        checkValidity(ref.buffers, valid);
        checkValidity(ref.images, valid);
        checkValidity(ref.samplers, valid);
    }
    return valid;
}

RenderHandleReference ShaderPipelineBinder::GetShaderHandle() const
{
    return shader_;
}

void ShaderPipelineBinder::Bind(const uint32_t set, const uint32_t binding, const RenderHandleReference& handle)
{
    const RenderHandleType type = handle.GetHandleType();
    if (type == RenderHandleType::GPU_BUFFER) {
        BindBuffer(set, binding, { handle, 0u, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE });
    } else if (type == RenderHandleType::GPU_IMAGE) {
        BindImage(set, binding,
            { handle, PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS,
                {} });
    } else if (type == RenderHandleType::GPU_SAMPLER) {
        BindSampler(set, binding, { handle });
    }
}

void ShaderPipelineBinder::SetUniformData(
    const uint32_t set, const uint32_t binding, const array_view<const uint8_t> data)
{}

void ShaderPipelineBinder::SetPushConstantData(const array_view<const uint8_t> data)
{
    if ((pipelineLayout_.pushConstant.byteSize > 0) && (!data.empty())) {
        const size_t maxByteSize = Math::min(pushData_.size_in_bytes(), data.size_bytes());
        CloneData(pushData_.data(), pushData_.size_in_bytes(), data.data(), maxByteSize);
    }
}

void ShaderPipelineBinder::BindBuffer(const uint32_t set, const uint32_t binding, const Buffer& resource)
{
    if (resource.handle && (set < descriptorSetResources_.size())) {
        const RenderHandleType type = resource.handle.GetHandleType();
        auto& setResources = descriptorSetResources_[set];
        if (binding < setResources.maxBindingCount) {
            const uint32_t idx = setResources.bindingToIndex[binding];
            if ((idx < setResources.bindings.size()) && (setResources.bindings[idx].type == type)) {
                const uint32_t resIdx = setResources.bindings[idx].resIdx;
                PLUGIN_ASSERT(resIdx < setResources.buffers.size());
                setResources.buffers[resIdx] = resource;
            }
        }
    }
}

void ShaderPipelineBinder::BindImage(const uint32_t set, const uint32_t binding, const Image& resource)
{
    if (resource.handle && (set < descriptorSetResources_.size())) {
        const RenderHandleType type = resource.handle.GetHandleType();
        auto& setResources = descriptorSetResources_[set];
        if (binding < setResources.maxBindingCount) {
            const uint32_t idx = setResources.bindingToIndex[binding];
            if ((idx < setResources.bindings.size()) && (setResources.bindings[idx].type == type)) {
                const uint32_t resIdx = setResources.bindings[idx].resIdx;
                PLUGIN_ASSERT(resIdx < setResources.images.size());
                setResources.images[resIdx] = resource;
            }
        }
    }
}

void ShaderPipelineBinder::BindSampler(const uint32_t set, const uint32_t binding, const Sampler& resource)
{
    if (resource.handle && (set < descriptorSetResources_.size())) {
        const RenderHandleType type = resource.handle.GetHandleType();
        auto& setResources = descriptorSetResources_[set];
        if (binding < setResources.maxBindingCount) {
            const uint32_t idx = setResources.bindingToIndex[binding];
            if ((idx < setResources.bindings.size()) && (setResources.bindings[idx].type == type)) {
                const uint32_t resIdx = setResources.bindings[idx].resIdx;
                PLUGIN_ASSERT(resIdx < setResources.samplers.size());
                setResources.samplers[resIdx] = resource;
            }
        }
    }
}

IShaderPipelineBinder::DescriptorSetView ShaderPipelineBinder::GetDescriptorSetView(const uint32_t set) const
{
    if (set < static_cast<uint32_t>(descriptorSetResources_.size())) {
        const auto& ref = descriptorSetResources_[set];
        return {
            ref.buffers,
            ref.images,
            ref.samplers,
            ref.bindings,
        };
    }
    return {};
}

array_view<const uint8_t> ShaderPipelineBinder::GetPushData() const
{
    return pushData_;
}
RENDER_END_NAMESPACE()
