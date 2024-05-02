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

#ifndef DEVICE_SHADER_PIPELINE_BINDER_H
#define DEVICE_SHADER_PIPELINE_BINDER_H

#include <atomic>

#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
#include <render/device/intf_shader_pipeline_binder.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "util/property_util.h"

RENDER_BEGIN_NAMESPACE()
class ShaderPipelineBinder;

class ShaderPipelineBinderPropertyBindingSignal final : public CustomPropertyWriteSignal {
public:
    explicit ShaderPipelineBinderPropertyBindingSignal(ShaderPipelineBinder& shaderPipelineBinder);
    ~ShaderPipelineBinderPropertyBindingSignal() override = default;

    void Signal() override;

private:
    ShaderPipelineBinder& shaderPipelineBinder_;
};

class ShaderPipelineBinder : public IShaderPipelineBinder {
public:
    ShaderPipelineBinder() = delete;
    ShaderPipelineBinder(
        IShaderManager& shaderMgr, const RenderHandleReference& shader, const PipelineLayout& pipelineLayout);
    ~ShaderPipelineBinder() override = default;

    void ClearBindings() override;
    bool GetBindingValidity() const override;
    RenderHandleReference GetShaderHandle() const override;

    void Bind(const uint32_t set, const uint32_t binding, const RenderHandleReference& handle) override;
    // Not yet in the API
    void SetUniformData(const uint32_t set, const uint32_t binding, const BASE_NS::array_view<const uint8_t> data);
    void SetPushConstantData(const BASE_NS::array_view<const uint8_t> data) override;

    void BindBuffer(
        const uint32_t set, const uint32_t binding, const BindableBufferWithHandleReference& resource) override;
    void BindBuffers(const uint32_t set, const uint32_t binding,
        BASE_NS::array_view<const BindableBufferWithHandleReference> resources) override;
    void BindImage(
        const uint32_t set, const uint32_t binding, const BindableImageWithHandleReference& resource) override;
    void BindImages(const uint32_t set, const uint32_t binding,
        BASE_NS::array_view<const BindableImageWithHandleReference> resources) override;
    void BindSampler(
        const uint32_t set, const uint32_t binding, const BindableSamplerWithHandleReference& resource) override;
    void BindSamplers(const uint32_t set, const uint32_t binding,
        BASE_NS::array_view<const BindableSamplerWithHandleReference> resources) override;

    void BindVertexBuffers(BASE_NS::array_view<const VertexBufferWithHandleReference> vertexBuffers) override;
    void BindIndexBuffer(const IndexBufferWithHandleReference& indexBuffer) override;
    void SetDrawCommand(const DrawCommand& drawCommand) override;
    void SetDispatchCommand(const DispatchCommand& dispatchCommand) override;

    DescriptorSetLayoutBindingResources GetDescriptorSetLayoutBindingResources(const uint32_t set) const override;

    BASE_NS::array_view<const uint8_t> GetPushConstantData() const override;

    BASE_NS::array_view<const VertexBufferWithHandleReference> GetVertexBuffers() const override;
    IndexBufferWithHandleReference GetIndexBuffer() const override;
    DrawCommand GetDrawCommand() const override;
    DispatchCommand GetDispatchCommand() const override;

    PipelineLayout GetPipelineLayout() const override;
    IPipelineDescriptorSetBinder* GetPipelineDescriptorSetBinder() const;

    CORE_NS::IPropertyHandle* GetProperties() override;
    CORE_NS::IPropertyHandle* GetBindingProperties() override;
    ResourceBinding GetResourceBinding(uint32_t set, uint32_t binding) const override;
    PropertyBindingView GetPropertyBindingView() const override;
    uint32_t GetPropertyBindingByteSize() const override;

    void BindPropertyBindings();

    struct CustomPropertyData {
        uint32_t set { ~0U };
        uint32_t binding { ~0U };

        BASE_NS::unique_ptr<CustomPropertyPodContainer> properties;
        CORE_NS::IPropertyHandle* handle { nullptr };
    };
    struct BindingPropertyData {
        struct SetAndBinding {
            uint32_t set { ~0U };
            uint32_t binding { ~0U };
        };

        BASE_NS::unique_ptr<ShaderPipelineBinderPropertyBindingSignal> bindingSignal;

        BASE_NS::unique_ptr<CustomPropertyBindingContainer> properties;
        CORE_NS::IPropertyHandle* handle { nullptr };
        BASE_NS::vector<SetAndBinding> setAndBindings;
    };

    struct BindableBinding {
        uint32_t binding { ~0U };
        uint32_t descriptorCount { ~0U };
        uint32_t resIdx { ~0U };
        uint32_t arrayoffset { ~0U };
        RenderHandleType type { RenderHandleType::UNDEFINED };
    };

protected:
    void Ref() override;
    void Unref() override;

private:
    void InitCustomProperties();
    void EvaluateCustomPropertyBindings();

    IShaderManager& shaderMgr_;

    std::atomic<int32_t> refcnt_ { 0 };

    RenderHandleReference shader_;
    PipelineLayout pipelineLayout_;

    RenderHandleType renderHandleType_ { RenderHandleType::UNDEFINED };

    DispatchCommand dispatchCommand_;
    DrawCommand drawCommand_;

    BASE_NS::vector<VertexBufferWithHandleReference> vertexBuffers_;
    IndexBufferWithHandleReference indexBuffer_;

    // NOTE: initial version with multiple vectors per descriptor set
    // vectors are resized in the constructor
    struct DescriptorSetResources {
        BASE_NS::vector<BindableBufferWithHandleReference> buffers;
        BASE_NS::vector<BindableImageWithHandleReference> images;
        BASE_NS::vector<BindableSamplerWithHandleReference> samplers;

        BASE_NS::vector<BindableBinding> bindings;

        uint8_t bindingToIndex[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT] { 16, 16, 16, 16, 16, 16, 16,
            16, 16, 16, 16, 16, 16, 16, 16, 16 };
    };
    BASE_NS::vector<DescriptorSetResources> descriptorSetResources_;

    BASE_NS::vector<uint8_t> pushData_;

    CustomPropertyData customPropertyData_;
    BindingPropertyData bindingPropertyData_;

    // real binder object which can be re-used immediately during rendering
    IPipelineDescriptorSetBinder::Ptr pipelineDescriptorSetBinder_;
    // set -> actual vector index
    uint32_t setToBinderIndex_[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] { ~0u, ~0u, ~0u, ~0u };
};
RENDER_END_NAMESPACE()

#endif // DEVICE_SHADER_PIPELINE_BINDER_H
