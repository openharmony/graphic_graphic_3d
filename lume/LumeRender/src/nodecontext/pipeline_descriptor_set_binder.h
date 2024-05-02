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

#ifndef RENDER_RENDER__PIPELINE_DESCRIPTOR_SET_BINDER_H
#define RENDER_RENDER__PIPELINE_DESCRIPTOR_SET_BINDER_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_handle_util.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
namespace DescriptorSetBinderUtil {
constexpr RenderHandleType GetRenderHandleType(const DescriptorType dt)
{
    if (dt == CORE_DESCRIPTOR_TYPE_SAMPLER) {
        return RenderHandleType::GPU_SAMPLER;
    } else if (((dt >= CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) && (dt <= CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE)) ||
               (dt == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)) {
        return RenderHandleType::GPU_IMAGE;
    } else if (((dt >= CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) &&
                (dt <= CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)) ||
               (dt == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE)) {
        return RenderHandleType::GPU_BUFFER;
    }
    return RenderHandleType::UNDEFINED;
}

constexpr PipelineStageFlags GetPipelineStageFlags(const ShaderStageFlags shaderStageFlags)
{
    PipelineStageFlags pipelineStageFlags { 0 };
    if (shaderStageFlags & ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT) {
        pipelineStageFlags |= PipelineStageFlagBits::CORE_PIPELINE_STAGE_VERTEX_SHADER_BIT;
    }
    if (shaderStageFlags & ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT) {
        pipelineStageFlags |= PipelineStageFlagBits::CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    if (shaderStageFlags & ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT) {
        pipelineStageFlags |= PipelineStageFlagBits::CORE_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    return pipelineStageFlags;
}

constexpr AccessFlags GetAccessFlags(const DescriptorType dt)
{
    if ((dt == CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) || (dt == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ||
        (dt == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)) {
        return CORE_ACCESS_UNIFORM_READ_BIT;
    } else if ((dt == CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) || (dt == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
               (dt == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) || (dt == CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE)) {
        // NOTE: could be optimized with shader reflection info
        return (CORE_ACCESS_SHADER_READ_BIT | CORE_ACCESS_SHADER_WRITE_BIT);
    } else if (dt == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
        return CORE_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    } else {
        // CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        // CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        // CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT
        // CORE_DESCRIPTOR_TYPE_SAMPLER
        // CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE
        return CORE_ACCESS_SHADER_READ_BIT;
    }
}
} // namespace DescriptorSetBinderUtil

/** DescriptorSetBinder.
 */
class DescriptorSetBinder final : public IDescriptorSetBinder {
public:
    DescriptorSetBinder(const RenderHandle handle,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings);
    explicit DescriptorSetBinder(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings);
    ~DescriptorSetBinder() = default;

    void ClearBindings() override;
    RenderHandle GetDescriptorSetHandle() const override;
    DescriptorSetLayoutBindingResources GetDescriptorSetLayoutBindingResources() const override;

    bool GetDescriptorSetLayoutBindingValidity() const override;
    void PrintDescriptorSetLayoutBindingValidation() const override;

    // all must call this
    void BindBuffer(
        const uint32_t binding, const BindableBuffer& resource, const AdditionalDescriptorFlags flags) override;
    void BindBuffer(const uint32_t binding, const BindableBuffer& resource) override;
    void BindBuffer(const uint32_t binding, const RenderHandle handle, const uint32_t byteOffset) override;
    void BindBuffer(
        const uint32_t binding, const RenderHandle handle, const uint32_t byteOffset, const uint32_t byteSize) override;
    // for descriptor array binding
    void BindBuffers(const uint32_t binding, const BASE_NS::array_view<const BindableBuffer> resources) override;

    // all must call this
    void BindImage(
        const uint32_t binding, const BindableImage& resource, const AdditionalDescriptorFlags flags) override;
    void BindImage(const uint32_t binding, const BindableImage& resource) override;
    void BindImage(const uint32_t binding, const RenderHandle handle) override;
    void BindImage(const uint32_t binding, const RenderHandle handle, const RenderHandle samplerHandle) override;
    // for descriptor array binding
    void BindImages(const uint32_t binding, const BASE_NS::array_view<const BindableImage> resources) override;

    // all must call this
    void BindSampler(
        const uint32_t binding, const BindableSampler& resource, const AdditionalDescriptorFlags flags) override;
    void BindSampler(const uint32_t binding, const BindableSampler& resource) override;
    void BindSampler(const uint32_t binding, const RenderHandle handle) override;
    // for descriptor array binding
    void BindSamplers(const uint32_t binding, const BASE_NS::array_view<const BindableSampler> resources) override;

protected:
    void Destroy() override;

private:
    void Init(const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings);
    void InitFillBindings(const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings,
        const uint32_t bufferCount, const uint32_t imageCount, const uint32_t samplerCount);

    RenderHandle handle_;
    BASE_NS::vector<DescriptorSetLayoutBindingResource> bindings_;

    BASE_NS::vector<BufferDescriptor> buffers_;
    BASE_NS::vector<ImageDescriptor> images_;
    BASE_NS::vector<SamplerDescriptor> samplers_;

    // accesses array with binding number and retuns index;
    PLUGIN_STATIC_ASSERT(PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT == 16u);
    uint8_t bindingToIndex_[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT] { 16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16 };
    uint32_t maxBindingCount_ { 0u };

    uint32_t descriptorBindingMask_ { 0 };
    uint32_t bindingMask_ { 0 };
};

/** PipelineDescriptorSetBinder.
 */
class PipelineDescriptorSetBinder final : public IPipelineDescriptorSetBinder {
public:
    PipelineDescriptorSetBinder(const PipelineLayout& pipelineLayout,
        const BASE_NS::array_view<const RenderHandle> handles,
        const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings);
    PipelineDescriptorSetBinder(const PipelineLayout& pipelineLayout,
        const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings);
    ~PipelineDescriptorSetBinder() = default;

    void ClearBindings() override;

    RenderHandle GetDescriptorSetHandle(const uint32_t set) const override;
    DescriptorSetLayoutBindingResources GetDescriptorSetLayoutBindingResources(const uint32_t set) const override;
    bool GetPipelineDescriptorSetLayoutBindingValidity() const override;

    uint32_t GetDescriptorSetCount() const override;
    BASE_NS::array_view<const uint32_t> GetSetIndices() const override;
    BASE_NS::array_view<const RenderHandle> GetDescriptorSetHandles() const override;

    uint32_t GetFirstSet() const override;
    BASE_NS::array_view<const RenderHandle> GetDescriptorSetHandles(
        const uint32_t beginSet, const uint32_t count) const override;

    void BindBuffer(const uint32_t set, const uint32_t binding, const BindableBuffer& resource,
        const AdditionalDescriptorFlags flags) override;
    void BindBuffer(const uint32_t set, const uint32_t binding, const BindableBuffer& resource) override;
    void BindBuffers(
        const uint32_t set, const uint32_t binding, const BASE_NS::array_view<const BindableBuffer> resources) override;

    void BindImage(const uint32_t set, const uint32_t binding, const BindableImage& resource,
        const AdditionalDescriptorFlags flags) override;
    void BindImage(const uint32_t set, const uint32_t binding, const BindableImage& resource) override;
    void BindImages(
        const uint32_t set, const uint32_t binding, const BASE_NS::array_view<const BindableImage> resources) override;

    void BindSampler(const uint32_t set, const uint32_t binding, const BindableSampler& resource,
        const AdditionalDescriptorFlags flags) override;
    void BindSampler(const uint32_t set, const uint32_t binding, const BindableSampler& resource) override;
    void BindSamplers(const uint32_t set, const uint32_t binding,
        const BASE_NS::array_view<const BindableSampler> resources) override;

    void PrintPipelineDescriptorSetLayoutBindingValidation() const override;

protected:
    void Destroy() override;

private:
    // binder can be created without valid handles
    void Init(const PipelineLayout& pipelineLayout, const BASE_NS::array_view<const RenderHandle> handles,
        const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings, bool validHandles);

    // set -> actual vector index
    uint32_t setToBinderIndex_[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] { ~0u, ~0u, ~0u, ~0u };

    BASE_NS::vector<DescriptorSetBinder> descriptorSetBinders_;

    BASE_NS::vector<uint32_t> setIndices_;
    BASE_NS::vector<RenderHandle> descriptorSetHandles_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__PIPELINE_DESCRIPTOR_SET_BINDER_H
