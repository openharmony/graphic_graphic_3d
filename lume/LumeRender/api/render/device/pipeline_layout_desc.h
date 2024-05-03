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

#ifndef API_RENDER_DEVICE_PIPELINE_LAYOUT_DESC_H
#define API_RENDER_DEVICE_PIPELINE_LAYOUT_DESC_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_render_pipelinelayoutdesc
 *  @{
 */
/** Pipeline layout constants */
struct PipelineLayoutConstants {
    /** Max descriptor set count */
    static constexpr uint32_t MAX_DESCRIPTOR_SET_COUNT { 4u };
    /** Max dynamic descriptor offset count */
    static constexpr uint32_t MAX_DYNAMIC_DESCRIPTOR_OFFSET_COUNT { 16u };
    /** Invalid index */
    static constexpr uint32_t INVALID_INDEX { ~0u };
    /** Max push constant byte size */
    static constexpr uint32_t MAX_PUSH_CONSTANT_BYTE_SIZE { 128u };
    /** Max binding count in a set */
    static constexpr uint32_t MAX_DESCRIPTOR_SET_BINDING_COUNT { 16u };
    /** Max push constant ranges */
    static constexpr uint32_t MAX_PUSH_CONSTANT_RANGE_COUNT { 1u };
    /** Max UBO bind byte size */
    static constexpr uint32_t MAX_UBO_BIND_BYTE_SIZE { 16u * 1024u };
    /** UBO bind offset alignemtn */
    static constexpr uint32_t MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE { 256u };
};

/** Descriptor set layout binding */
struct DescriptorSetLayoutBinding {
    /** Binding */
    uint32_t binding { PipelineLayoutConstants::INVALID_INDEX };
    /** Descriptor type */
    DescriptorType descriptorType { DescriptorType::CORE_DESCRIPTOR_TYPE_MAX_ENUM };
    /** Descriptor count */
    uint32_t descriptorCount { 0 };
    /** Stage flags */
    ShaderStageFlags shaderStageFlags { 0 };
};

/** Descriptor set layout bindings */
struct DescriptorSetLayoutBindings {
    /** Bindings array */
    BASE_NS::vector<DescriptorSetLayoutBinding> binding;
};

/** Descriptor set layout binding resource */
struct DescriptorSetLayoutBindingResource {
    /** Binding */
    DescriptorSetLayoutBinding binding;
    /** Resource index to typed data */
    uint32_t resourceIndex { PipelineLayoutConstants::INVALID_INDEX };
};

/** Bindable buffer */
struct BindableBuffer {
    /** Handle */
    RenderHandle handle {};
    /** Byte offset to buffer */
    uint32_t byteOffset { 0u };
    /** Byte size for buffer binding */
    uint32_t byteSize { PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
};

/** Bindable image */
struct BindableImage {
    /** Handle */
    RenderHandle handle {};
    /** Mip level for specific binding */
    uint32_t mip { PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS };
    /** Layer level for specific binding */
    uint32_t layer { PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    /** Custom image layout */
    ImageLayout imageLayout { ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
    /** Sampler handle for combined image sampler */
    RenderHandle samplerHandle {};
};

/** Bindable sampler */
struct BindableSampler {
    /** Handle */
    RenderHandle handle {};
};

/** Bindable buffer with render handle reference */
struct BindableBufferWithHandleReference {
    /** Handle */
    RenderHandleReference handle;
    /** Byte offset to buffer */
    uint32_t byteOffset { 0u };
    /** Byte size for buffer binding */
    uint32_t byteSize { PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
};

/** Bindable image with render handle reference */
struct BindableImageWithHandleReference {
    /** Handle */
    RenderHandleReference handle;
    /** Mip level for specific binding */
    uint32_t mip { PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS };
    /** Layer level for specific binding */
    uint32_t layer { PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    /** Custom image layout */
    ImageLayout imageLayout { ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
    /** Sampler handle for combined image sampler */
    RenderHandleReference samplerHandle;
};

/** Bindable with sampler handle reference */
struct BindableSamplerWithHandleReference {
    /** Handle */
    RenderHandleReference handle;
};

/** Descriptor structure for buffer */
struct BufferDescriptor {
    /** Descriptor set layout binding */
    DescriptorSetLayoutBinding binding {};
    /** Bindable resource structure with handle */
    BindableBuffer resource {};
    /** Resource state in the pipeline */
    GpuResourceState state {};

    /** Array offset to resources for array descriptors */
    uint32_t arrayOffset { 0 };

    /** Additional flags */
    AdditionalDescriptorFlags additionalFlags { 0u };
};

/** Descriptor structure for image */
struct ImageDescriptor {
    /** Descriptor set layout binding */
    DescriptorSetLayoutBinding binding {};
    /** Bindable resource structure with handle */
    BindableImage resource {};
    /** Resource state in the pipeline */
    GpuResourceState state {};

    /** Array offset to resources for array descriptors */
    uint32_t arrayOffset { 0 };

    /** Additional flags */
    AdditionalDescriptorFlags additionalFlags { 0u };
};

/** Descriptor structure for sampler */
struct SamplerDescriptor {
    /** Descriptor set layout binding */
    DescriptorSetLayoutBinding binding {};
    /** Bindable resource structure with handle */
    BindableSampler resource {};

    /** Array offset to resources for array descriptors */
    uint32_t arrayOffset { 0 };

    /** Additional flags */
    AdditionalDescriptorFlags additionalFlags { 0u };
};

/** Descriptor set layout binding resources */
struct DescriptorSetLayoutBindingResources {
    /** Bindings */
    BASE_NS::array_view<const DescriptorSetLayoutBindingResource> bindings;

    /** Buffer descriptors */
    BASE_NS::array_view<const BufferDescriptor> buffers;
    /** Image descriptors */
    BASE_NS::array_view<const ImageDescriptor> images;
    /** Sampler descriptors */
    BASE_NS::array_view<const SamplerDescriptor> samplers;

    /** Mask of bindings in the descriptor set. Max uint is value which means that not set */
    uint32_t descriptorSetBindingMask { ~0u };
    /** Current binding mask. Max uint is value which means that not set */
    uint32_t bindingMask { ~0u };
};

/** Descriptor set layout */
struct DescriptorSetLayout {
    /** Set */
    uint32_t set { PipelineLayoutConstants::INVALID_INDEX };
    /** Bindings */
    BASE_NS::vector<DescriptorSetLayoutBinding> bindings;
};

/** Push constant */
struct PushConstant {
    /** Shader stage flags */
    ShaderStageFlags shaderStageFlags { 0 };
    /** Byte size */
    uint32_t byteSize { 0 };
};

/** Pipeline layout */
struct PipelineLayout {
    /** Push constant */
    PushConstant pushConstant;
    /** Descriptor set count */
    uint32_t descriptorSetCount { 0 };
    /** Descriptor sets */
    DescriptorSetLayout descriptorSetLayouts[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] {};
};

/** Shader thread group variables (can be used with group count and group sizes */
struct ShaderThreadGroup {
    /** Thread group variable X */
    uint32_t x { 1u };
    /** Thread group variable Y */
    uint32_t y { 1u };
    /** Thread group variable Z */
    uint32_t z { 1u };
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_DEVICE_PIPELINE_LAYOUT_DESC_H
