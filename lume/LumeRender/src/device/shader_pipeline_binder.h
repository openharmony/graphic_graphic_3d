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

#ifndef DEVICE_SHADER_PIPELINE_BINDER_H
#define DEVICE_SHADER_PIPELINE_BINDER_H

#include <atomic>

#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
#include <render/device/intf_shader_pipeline_binder.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class ShaderPipelineBinder : public IShaderPipelineBinder {
public:
    ShaderPipelineBinder() = delete;
    ShaderPipelineBinder(const RenderHandleReference& shader, const PipelineLayout& pipelineLayout);
    ~ShaderPipelineBinder() override = default;

    bool GetBindingValidity() const override;
    RenderHandleReference GetShaderHandle() const override;

    void Bind(const uint32_t set, const uint32_t binding, const RenderHandleReference& handle) override;
    void SetUniformData(
        const uint32_t set, const uint32_t binding, const BASE_NS::array_view<const uint8_t> data) override;
    void SetPushConstantData(const BASE_NS::array_view<const uint8_t> data) override;

    void BindBuffer(const uint32_t set, const uint32_t binding, const Buffer& handle) override;
    void BindImage(const uint32_t set, const uint32_t binding, const Image& handle) override;
    void BindSampler(const uint32_t set, const uint32_t binding, const Sampler& resource) override;

    DescriptorSetView GetDescriptorSetView(const uint32_t set) const override;
    BASE_NS::array_view<const uint8_t> GetPushData() const override;

protected:
    void Ref() override;
    void Unref() override;

private:
    std::atomic<int32_t> refcnt_ { 0 };

    RenderHandleReference shader_;
    PipelineLayout pipelineLayout_;

    // NOTE: initial version with multiple vectors per descriptor set
    // vectors are resized in the constructor
    struct DescriptorSetResources {
        BASE_NS::vector<Buffer> buffers;
        BASE_NS::vector<Image> images;
        BASE_NS::vector<Sampler> samplers;

        BASE_NS::vector<Binding> bindings;

        uint8_t bindingToIndex[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT] { 16, 16, 16, 16, 16, 16, 16,
            16, 16, 16, 16, 16, 16, 16, 16, 16 };
        uint32_t maxBindingCount { 0u };
    };
    BASE_NS::vector<DescriptorSetResources> descriptorSetResources_;

    BASE_NS::vector<uint8_t> pushData_;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_SHADER_PIPELINE_BINDER_H
