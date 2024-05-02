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

#include "shader_pipeline_binder.h"

#include <algorithm>
#include <atomic>

#include <base/containers/allocator.h>
#include <base/containers/array_view.h>
#include <base/math/matrix_util.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/property/property_types.h>

#include "nodecontext/pipeline_descriptor_set_binder.h"
#include "util/log.h"
#include "util/property_util.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t CUSTOM_PROPERTY_POD_CONTAINER_BYTE_SIZE { 256U };
constexpr string_view CUSTOM_PROPERTIES = "customProperties";
constexpr string_view CUSTOM_BINDING_PROPERTIES = "customBindingProperties";
constexpr string_view BINDING_PROPERTIES = "bindingProperties";
constexpr string_view CUSTOM_PROPERTY_DATA = "data";
constexpr string_view NAME = "name";
constexpr string_view DISPLAY_NAME = "displayName";
constexpr string_view DESCRIPTOR_SET = "set";
constexpr string_view DESCRIPTOR_SET_BINDING = "binding";

void UpdateCustomPropertyMetadata(const json::value& customProperties, ShaderPipelineBinder::CustomPropertyData& cpd)
{
    if (customProperties && customProperties.is_array() && cpd.properties) {
        CustomPropertyPodContainer& properties = *cpd.properties;
        for (const auto& ref : customProperties.array_) {
            if (const auto customProps = ref.find(CUSTOM_PROPERTIES); customProps && customProps->is_array()) {
                // process custom properties i.e. local factors
                for (const auto& propRef : customProps->array_) {
                    if (const auto setProp = propRef.find(DESCRIPTOR_SET); setProp && setProp->is_number()) {
                        cpd.set = (uint32_t)setProp->unsigned_;
                    }
                    if (const auto setProp = propRef.find(DESCRIPTOR_SET_BINDING); setProp && setProp->is_number()) {
                        cpd.binding = (uint32_t)setProp->unsigned_;
                    }
                    if (const auto customData = propRef.find(CUSTOM_PROPERTY_DATA); customData) {
                        // reserve the property count
                        properties.ReservePropertyCount(customData->array_.size());
                        for (const auto& dataValue : customData->array_) {
                            if (dataValue.is_object()) {
                                string_view name;
                                string_view displayName;
                                string_view type;
                                const json::value* value = nullptr;
                                for (const auto& dataObject : dataValue.object_) {
                                    if (dataObject.key == NAME && dataObject.value.is_string()) {
                                        name = dataObject.value.string_;
                                    } else if (dataObject.key == DISPLAY_NAME && dataObject.value.is_string()) {
                                        displayName = dataObject.value.string_;
                                    } else if (dataObject.key == "type" && dataObject.value.is_string()) {
                                        type = dataObject.value.string_;
                                    } else if (dataObject.key == "value") {
                                        value = &dataObject.value;
                                    }
                                }
                                const PropertyTypeDecl typeDecl =
                                    CustomPropertyPodHelper::GetPropertyTypeDeclaration(type);
                                const size_t align = CustomPropertyPodHelper::GetPropertyTypeAlignment(typeDecl);
                                const size_t offset = [](size_t value, size_t align) -> size_t {
                                    if (align == 0U) {
                                        return value;
                                    }
                                    return ((value + align - 1U) / align) * align;
                                }(properties.GetByteSize(), align);

                                properties.AddOffsetProperty(name, displayName, offset, typeDecl);
                                CustomPropertyPodHelper::SetCustomPropertyBlobValue(
                                    typeDecl, value, properties, offset);
                            }
                        }
                    }
                }
            }
        }
    }
}

void UpdateBindingPropertyMetadata(const json::value& customProperties, CustomPropertyBindingContainer& properties,
    vector<ShaderPipelineBinder::BindingPropertyData::SetAndBinding>& setAndBindings)
{
    if (customProperties && customProperties.is_array()) {
        for (const auto& ref : customProperties.array_) {
            auto customProps = ref.find(BINDING_PROPERTIES);
            if (!customProps) {
                customProps = ref.find(CUSTOM_BINDING_PROPERTIES);
            }
            if (customProps && customProps->is_array()) {
                setAndBindings.reserve(customProps->array_.size());
                // process the array
                for (const auto& propRef : customProps->array_) {
                    if (const auto customData = propRef.find(CUSTOM_PROPERTY_DATA); customData) {
                        // reserve the property count
                        properties.ReservePropertyCount(customData->array_.size());
                        for (const auto& dataValue : customData->array_) {
                            if (dataValue.is_object()) {
                                string_view name;
                                string_view displayName;
                                string_view type;
                                uint32_t set { ~0U };
                                uint32_t binding { ~0U };
                                for (const auto& dataObject : dataValue.object_) {
                                    if (dataObject.key == NAME && dataObject.value.is_string()) {
                                        name = dataObject.value.string_;
                                    } else if (dataObject.key == DISPLAY_NAME && dataObject.value.is_string()) {
                                        displayName = dataObject.value.string_;
                                    } else if (dataObject.key == "type" && dataObject.value.is_string()) {
                                        type = dataObject.value.string_;
                                    } else if (dataObject.key == DESCRIPTOR_SET && dataObject.value.is_number()) {
                                        set = (uint32_t)dataObject.value.unsigned_;
                                    } else if (dataObject.key == DESCRIPTOR_SET_BINDING &&
                                               dataObject.value.is_number()) {
                                        binding = (uint32_t)dataObject.value.unsigned_;
                                    }
                                }
                                set = Math::min(set, PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT);
                                binding = Math::min(binding, PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT);
                                setAndBindings.push_back({ set, binding });
                                const PropertyTypeDecl typeDecl =
                                    CustomPropertyBindingHelper::GetPropertyTypeDeclaration(type);
                                const size_t align = CustomPropertyBindingHelper::GetPropertyTypeAlignment(typeDecl);
                                const size_t offset = [](size_t value, size_t align) -> size_t {
                                    if (align == 0U) {
                                        return value;
                                    }
                                    return ((value + align - 1U) / align) * align;
                                }(properties.GetByteSize(), align);
                                properties.AddOffsetProperty(name, displayName, offset, typeDecl);
                            }
                        }
                    }
                }
            }
        }
    }
}

IPipelineDescriptorSetBinder::Ptr CreatePipelineDescriptorSetBinder(const PipelineLayout& pipelineLayout)
{
    DescriptorSetLayoutBindings descriptorSetLayoutBindings[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT];
    for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
        if (pipelineLayout.descriptorSetLayouts[idx].set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
            descriptorSetLayoutBindings[idx] = { pipelineLayout.descriptorSetLayouts[idx].bindings };
        }
    }
    // pass max amount to binder, it will check validity of sets and their set indices
    return IPipelineDescriptorSetBinder::Ptr { new PipelineDescriptorSetBinder(
        pipelineLayout, descriptorSetLayoutBindings) };
}

struct DescriptorCountValues {
    uint32_t count { 0U };
    uint32_t arrayCount { 0U };
};
} // namespace

ShaderPipelineBinderPropertyBindingSignal::ShaderPipelineBinderPropertyBindingSignal(
    ShaderPipelineBinder& shaderPipelineBinder)
    : shaderPipelineBinder_(shaderPipelineBinder)
{}

void ShaderPipelineBinderPropertyBindingSignal::Signal()
{
    shaderPipelineBinder_.BindPropertyBindings();
}

ShaderPipelineBinder::ShaderPipelineBinder(
    IShaderManager& shaderMgr, const RenderHandleReference& shader, const PipelineLayout& pipelineLayout)
    : shaderMgr_(shaderMgr), shader_(shader), pipelineLayout_(pipelineLayout), renderHandleType_(shader.GetHandleType())
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!((renderHandleType_ == RenderHandleType::SHADER_STATE_OBJECT) ||
            (renderHandleType_ == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT))) {
        PLUGIN_LOG_W("RENDER_VALIDATION: Invalid handle for shader pipeline binder (type:%hhu)", renderHandleType_);
    }
#endif
    InitCustomProperties();
    pipelineDescriptorSetBinder_ = CreatePipelineDescriptorSetBinder(pipelineLayout_);
    // process with pipeline descriptor set binder
    if (pipelineDescriptorSetBinder_) {
        const array_view<const uint32_t> setIndices = pipelineDescriptorSetBinder_->GetSetIndices();
        descriptorSetResources_.resize(setIndices.size());
        uint32_t vecIdx = 0U;
        for (const auto& setIdx : setIndices) {
            const DescriptorSetLayoutBindingResources descSetBindingRes =
                pipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(setIdx);
            auto& descSetRes = descriptorSetResources_[vecIdx];
            // resize
            descSetRes.bindings.resize(descSetBindingRes.bindings.size());
            descSetRes.buffers.resize(descSetBindingRes.buffers.size());
            descSetRes.images.resize(descSetBindingRes.images.size());
            descSetRes.samplers.resize(descSetBindingRes.samplers.size());
            // set bindings
            for (size_t idx = 0; idx < descSetBindingRes.bindings.size(); ++idx) {
                const auto& ref = descSetBindingRes.bindings[idx];
                if (ref.binding.binding >= PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
                    continue;
                }
                // indirection
                descSetRes.bindingToIndex[ref.binding.binding] = static_cast<uint8_t>(idx);

                const RenderHandleType type = DescriptorSetBinderUtil::GetRenderHandleType(ref.binding.descriptorType);
                descSetRes.bindings[idx].binding = ref.binding.binding;
                descSetRes.bindings[idx].resIdx = ref.resourceIndex;
                descSetRes.bindings[idx].type = type;
                descSetRes.bindings[idx].descriptorCount = ref.binding.descriptorCount;
                if (type == RenderHandleType::GPU_BUFFER) {
                    descSetRes.bindings[idx].arrayoffset = descSetBindingRes.buffers[ref.resourceIndex].arrayOffset;
                } else if (type == RenderHandleType::GPU_IMAGE) {
                    descSetRes.bindings[idx].arrayoffset = descSetBindingRes.images[ref.resourceIndex].arrayOffset;
                } else if (type == RenderHandleType::GPU_SAMPLER) {
                    descSetRes.bindings[idx].arrayoffset = descSetBindingRes.samplers[ref.resourceIndex].arrayOffset;
                }
            }
            setToBinderIndex_[setIdx] = vecIdx++;
        }
    }
    if (pipelineLayout_.pushConstant.byteSize > 0) {
        pushData_.resize(pipelineLayout_.pushConstant.byteSize);
        std::fill(pushData_.begin(), pushData_.end(), uint8_t(0));
    }
    EvaluateCustomPropertyBindings();
}

void ShaderPipelineBinder::InitCustomProperties()
{
    // fetch custom properties if any
    if (const auto* metaJson = shaderMgr_.GetMaterialMetadata(shader_); (metaJson && metaJson->is_array())) {
        customPropertyData_.properties =
            make_unique<CustomPropertyPodContainer>(CUSTOM_PROPERTY_POD_CONTAINER_BYTE_SIZE);
        bindingPropertyData_.bindingSignal = make_unique<ShaderPipelineBinderPropertyBindingSignal>(*this);
        if (bindingPropertyData_.bindingSignal) {
            bindingPropertyData_.properties =
                make_unique<CustomPropertyBindingContainer>(*bindingPropertyData_.bindingSignal);
        }
        if (customPropertyData_.properties && bindingPropertyData_.properties) {
            UpdateCustomPropertyMetadata(*metaJson, customPropertyData_);
            customPropertyData_.handle = customPropertyData_.properties.get();

            UpdateBindingPropertyMetadata(
                *metaJson, *bindingPropertyData_.properties, bindingPropertyData_.setAndBindings);
            bindingPropertyData_.handle = bindingPropertyData_.properties.get();
        }
    }
}

void ShaderPipelineBinder::EvaluateCustomPropertyBindings()
{
#if (RENDER_VALIDATION_ENABLED == 1)
    for (uint32_t setIdx = 0; setIdx < static_cast<uint32_t>(descriptorSetResources_.size()); ++setIdx) {
        const auto& descRef = descriptorSetResources_[setIdx];
        const auto& plSet = pipelineLayout_.descriptorSetLayouts[setIdx];
        for (const auto& bindingRef : descRef.bindings) {
            bool valid = (bindingRef.binding < static_cast<uint32_t>(plSet.bindings.size()));
            if (valid) {
                const RenderHandleType plDescType =
                    DescriptorSetBinderUtil::GetRenderHandleType(plSet.bindings[bindingRef.binding].descriptorType);
                valid = valid && (bindingRef.type != plDescType);
            }
            if (valid) {
                CORE_LOG_W("RENDER_VALIDATION: Binding property descriptor set binding missmatch to pipeline layout "
                           "(set: %u, binding: %u)",
                    setIdx, bindingRef.binding);
            }
        }
    }
#endif
}

void ShaderPipelineBinder::ClearBindings()
{
    auto ClearResourcesBindings = [](auto& resources) {
        for (auto& ref : resources) {
            ref = {};
        }
    };

    for (auto& descRef : descriptorSetResources_) {
        ClearResourcesBindings(descRef.buffers);
        ClearResourcesBindings(descRef.images);
        ClearResourcesBindings(descRef.samplers);
    }
    if (pipelineDescriptorSetBinder_) {
        pipelineDescriptorSetBinder_->ClearBindings();
    }
}

bool ShaderPipelineBinder::GetBindingValidity() const
{
    bool valid = true;
    auto checkValidity = [](const auto& vec, bool& valid) {
        for (const auto& ref : vec) {
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
{
    // not yet supported
    PLUGIN_ASSERT(false);
}

void ShaderPipelineBinder::SetPushConstantData(const array_view<const uint8_t> data)
{
    if ((pipelineLayout_.pushConstant.byteSize > 0) && (!data.empty())) {
        const size_t maxByteSize = Math::min(pushData_.size_in_bytes(), data.size_bytes());
        CloneData(pushData_.data(), pushData_.size_in_bytes(), data.data(), maxByteSize);
    }
}

void ShaderPipelineBinder::BindBuffer(
    const uint32_t set, const uint32_t binding, const BindableBufferWithHandleReference& resource)
{
    if (resource.handle && set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetResources_.size())) {
            const RenderHandleType type = resource.handle.GetHandleType();
            auto& setResources = descriptorSetResources_[setIdx];
            bool validBinding = false;
            if (binding < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
                const uint32_t idx = setResources.bindingToIndex[binding];
                if ((idx < setResources.bindings.size()) && (setResources.bindings[idx].type == type)) {
                    const uint32_t resIdx = setResources.bindings[idx].resIdx;
                    if (resIdx < setResources.buffers.size()) {
                        validBinding = true;
                        setResources.buffers[resIdx] = resource;
                    }
                }
            }
            if (validBinding && pipelineDescriptorSetBinder_) {
                const BindableBuffer bindable {
                    resource.handle.GetHandle(),
                    resource.byteOffset,
                    resource.byteSize,
                };
                pipelineDescriptorSetBinder_->BindBuffer(set, binding, bindable);
            }
        }
    }
}

void ShaderPipelineBinder::BindBuffers(
    const uint32_t set, const uint32_t binding, const array_view<const BindableBufferWithHandleReference> resources)
{
    if ((!resources.empty()) && set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetResources_.size())) {
            const RenderHandleType type = resources[0].handle.GetHandleType();
            auto& setResources = descriptorSetResources_[setIdx];
            bool validBinding = false;
            if (binding < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
                const uint32_t idx = setResources.bindingToIndex[binding];
                if ((idx < setResources.bindings.size()) && (setResources.bindings[idx].type == type)) {
                    const uint32_t resIdx = setResources.bindings[idx].resIdx;
                    if (resIdx < setResources.buffers.size()) {
                        validBinding = true;
                        setResources.buffers[resIdx] = resources[0];
                    }
                }
            }
            if (validBinding && pipelineDescriptorSetBinder_) {
                vector<BindableBuffer> bindables;
                bindables.resize(resources.size());
                for (size_t idx = 0; idx < resources.size(); ++idx) {
                    const auto& rRef = resources[idx];
                    bindables[idx] = BindableBuffer { rRef.handle.GetHandle(), rRef.byteOffset, rRef.byteSize };
                }
                pipelineDescriptorSetBinder_->BindBuffers(set, binding, bindables);
            }
        }
    }
}

void ShaderPipelineBinder::BindImage(
    const uint32_t set, const uint32_t binding, const BindableImageWithHandleReference& resource)
{
    if (resource.handle && (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT)) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetResources_.size())) {
            const RenderHandleType type = resource.handle.GetHandleType();
            auto& setResources = descriptorSetResources_[setIdx];
            bool validBinding = false;
            if (binding < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
                const uint32_t idx = setResources.bindingToIndex[binding];
                if ((idx < setResources.bindings.size()) && (setResources.bindings[idx].type == type)) {
                    const uint32_t resIdx = setResources.bindings[idx].resIdx;
                    if (resIdx < setResources.images.size()) {
                        validBinding = true;
                        setResources.images[resIdx] = resource;
                    }
                }
            }
            if (validBinding && pipelineDescriptorSetBinder_) {
                const BindableImage bindable {
                    resource.handle.GetHandle(),
                    resource.mip,
                    resource.layer,
                    ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED,
                    resource.samplerHandle.GetHandle(),
                };
                pipelineDescriptorSetBinder_->BindImage(set, binding, bindable);
            }
        }
    }
}

void ShaderPipelineBinder::BindImages(
    const uint32_t set, const uint32_t binding, array_view<const BindableImageWithHandleReference> resources)
{
    if ((!resources.empty()) && (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT)) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetResources_.size())) {
            const RenderHandleType type = resources[0U].handle.GetHandleType();
            auto& setResources = descriptorSetResources_[setIdx];
            bool validBinding = false;
            if (binding < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
                const uint32_t idx = setResources.bindingToIndex[binding];
                if ((idx < setResources.bindings.size()) && (setResources.bindings[idx].type == type)) {
                    const uint32_t resIdx = setResources.bindings[idx].resIdx;
                    if (resIdx < setResources.images.size()) {
                        validBinding = true;
                        setResources.images[resIdx] = resources[0U];
                    }
                }
            }
            if (validBinding && pipelineDescriptorSetBinder_) {
                vector<BindableImage> bindables;
                bindables.resize(resources.size());
                for (size_t idx = 0; idx < resources.size(); ++idx) {
                    const auto& rRef = resources[idx];
                    bindables[idx] = BindableImage { rRef.handle.GetHandle(), rRef.mip, rRef.layer,
                        ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED, rRef.samplerHandle.GetHandle() };
                }
                pipelineDescriptorSetBinder_->BindImages(set, binding, bindables);
            }
        }
    }
}

void ShaderPipelineBinder::BindSampler(
    const uint32_t set, const uint32_t binding, const BindableSamplerWithHandleReference& resource)
{
    if (resource.handle && (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT)) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetResources_.size())) {
            const RenderHandleType type = resource.handle.GetHandleType();
            auto& setResources = descriptorSetResources_[setIdx];
            bool validBinding = false;
            if (binding < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
                const uint32_t idx = setResources.bindingToIndex[binding];
                if ((idx < setResources.bindings.size()) && (setResources.bindings[idx].type == type)) {
                    const uint32_t resIdx = setResources.bindings[idx].resIdx;
                    if (resIdx < setResources.samplers.size()) {
                        validBinding = true;
                        setResources.samplers[resIdx] = resource;
                    }
                }
            }
            if (validBinding && pipelineDescriptorSetBinder_) {
                const BindableSampler bindable {
                    resource.handle.GetHandle(),
                };
                pipelineDescriptorSetBinder_->BindSampler(set, binding, bindable);
            }
        }
    }
}

void ShaderPipelineBinder::BindSamplers(
    const uint32_t set, const uint32_t binding, array_view<const BindableSamplerWithHandleReference> resources)
{
    if ((!resources.empty()) && (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT)) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetResources_.size())) {
            const RenderHandleType type = resources[0U].handle.GetHandleType();
            auto& setResources = descriptorSetResources_[setIdx];
            bool validBinding = false;
            if (binding < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
                const uint32_t idx = setResources.bindingToIndex[binding];
                if ((idx < setResources.bindings.size()) && (setResources.bindings[idx].type == type)) {
                    const uint32_t resIdx = setResources.bindings[idx].resIdx;
                    if (resIdx < setResources.samplers.size()) {
                        validBinding = true;
                        setResources.samplers[resIdx] = resources[0U];
                    }
                }
            }
            if (validBinding && pipelineDescriptorSetBinder_) {
                vector<BindableSampler> bindables;
                bindables.resize(resources.size());
                for (size_t idx = 0; idx < resources.size(); ++idx) {
                    const auto& rRef = resources[idx];
                    bindables[idx] = BindableSampler { rRef.handle.GetHandle() };
                }
                pipelineDescriptorSetBinder_->BindSamplers(set, binding, bindables);
            }
        }
    }
}

void ShaderPipelineBinder::BindVertexBuffers(BASE_NS::array_view<const VertexBufferWithHandleReference> vertexBuffers)
{
    vertexBuffers_.clear();
    vertexBuffers_.append(vertexBuffers.begin(), vertexBuffers.end());
}

void ShaderPipelineBinder::BindIndexBuffer(const IndexBufferWithHandleReference& indexBuffer)
{
    indexBuffer_ = indexBuffer;
}

void ShaderPipelineBinder::SetDrawCommand(const DrawCommand& drawCommand)
{
    drawCommand_ = drawCommand;
}

void ShaderPipelineBinder::SetDispatchCommand(const DispatchCommand& dispatchCommand)
{
    dispatchCommand_ = dispatchCommand;
}

DescriptorSetLayoutBindingResources ShaderPipelineBinder::GetDescriptorSetLayoutBindingResources(
    const uint32_t set) const
{
    if (pipelineDescriptorSetBinder_) {
        return pipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(set);
    } else {
        return {};
    }
}

array_view<const uint8_t> ShaderPipelineBinder::GetPushConstantData() const
{
    return pushData_;
}

IPropertyHandle* ShaderPipelineBinder::GetProperties()
{
    return customPropertyData_.handle;
}

IPropertyHandle* ShaderPipelineBinder::GetBindingProperties()
{
    return bindingPropertyData_.handle;
}

IShaderPipelineBinder::ResourceBinding ShaderPipelineBinder::GetResourceBinding(uint32_t set, uint32_t binding) const
{
    if (set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        const uint32_t setIdx = setToBinderIndex_[set];
        if (setIdx < static_cast<uint32_t>(descriptorSetResources_.size())) {
            const auto& setResources = descriptorSetResources_[setIdx];
            if (binding < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_BINDING_COUNT) {
                const uint32_t idx = setResources.bindingToIndex[binding];
                if (idx < setResources.bindings.size()) {
                    IShaderPipelineBinder::ResourceBinding rb;
                    const auto& resBinding = setResources.bindings[idx];
                    rb.set = set;
                    rb.binding = binding;
                    rb.arrayOffset = resBinding.arrayoffset;
                    rb.descriptorCount = resBinding.descriptorCount;
                    if ((resBinding.type == RenderHandleType::GPU_BUFFER) &&
                        (resBinding.resIdx < setResources.buffers.size())) {
                        rb.handle = setResources.buffers[resBinding.resIdx].handle;
                    } else if ((resBinding.type == RenderHandleType::GPU_IMAGE) &&
                               (resBinding.resIdx < setResources.images.size())) {
                        rb.handle = setResources.images[resBinding.resIdx].handle;
                    } else if ((resBinding.type == RenderHandleType::GPU_SAMPLER) &&
                               (resBinding.resIdx < setResources.samplers.size())) {
                        rb.handle = setResources.samplers[resBinding.resIdx].handle;
                    }
                    return rb;
                }
            }
        }
    }
    return {};
}

IShaderPipelineBinder::PropertyBindingView ShaderPipelineBinder::GetPropertyBindingView() const
{
    PropertyBindingView cpbv;
    cpbv.set = customPropertyData_.set;
    cpbv.binding = customPropertyData_.binding;
    cpbv.data =
        (customPropertyData_.properties) ? customPropertyData_.properties->GetData() : array_view<const uint8_t> {};
    return cpbv;
}

uint32_t ShaderPipelineBinder::GetPropertyBindingByteSize() const
{
    return (customPropertyData_.properties) ? static_cast<uint32_t>(customPropertyData_.properties->GetByteSize()) : 0U;
}

array_view<const VertexBufferWithHandleReference> ShaderPipelineBinder::GetVertexBuffers() const
{
    return vertexBuffers_;
}

IndexBufferWithHandleReference ShaderPipelineBinder::GetIndexBuffer() const
{
    return indexBuffer_;
}

IShaderPipelineBinder::DrawCommand ShaderPipelineBinder::GetDrawCommand() const
{
    return drawCommand_;
}

IShaderPipelineBinder::DispatchCommand ShaderPipelineBinder::GetDispatchCommand() const
{
    return dispatchCommand_;
}

void ShaderPipelineBinder::BindPropertyBindings()
{
    // fetch the bindings from properties, and try to bind them
    if (bindingPropertyData_.properties && (bindingPropertyData_.properties->PropertyCount() > 0)) {
        auto* bindingProperties = bindingPropertyData_.properties.get();
        PLUGIN_ASSERT(bindingProperties->Owner()->MetaData().size() == bindingPropertyData_.setAndBindings.size());
        for (size_t idx = 0; idx < bindingProperties->Owner()->MetaData().size(); ++idx) {
            // the ordering should match with the bindings in the .shader file
            const auto& prop = bindingProperties->Owner()->MetaData()[idx];
            const auto& sb = bindingPropertyData_.setAndBindings[idx];
            switch (prop.type) {
                case PropertyType::BINDABLE_BUFFER_WITH_HANDLE_REFERENCE_T: {
                    BindBuffer(sb.set, sb.binding, bindingProperties->GetValue<BindableBufferWithHandleReference>(idx));
                }
                    break;
                case PropertyType::BINDABLE_IMAGE_WITH_HANDLE_REFERENCE_T: {
                    BindImage(sb.set, sb.binding, bindingProperties->GetValue<BindableImageWithHandleReference>(idx));
                }
                    break;
                case PropertyType::BINDABLE_SAMPLER_WITH_HANDLE_REFERENCE_T: {
                    BindSampler(
                        sb.set, sb.binding, bindingProperties->GetValue<BindableSamplerWithHandleReference>(idx));
                }
                    break;
                default: {
                }
                    break;
            }
        }
    }
}

PipelineLayout ShaderPipelineBinder::GetPipelineLayout() const
{
    return pipelineLayout_;
}

IPipelineDescriptorSetBinder* ShaderPipelineBinder::GetPipelineDescriptorSetBinder() const
{
    if (pipelineDescriptorSetBinder_) {
        return pipelineDescriptorSetBinder_.get();
    } else {
        return nullptr;
    }
}

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
RENDER_END_NAMESPACE()
