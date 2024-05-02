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

#ifndef RENDER_RENDER__NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_H
#define RENDER_RENDER__NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_handle_util.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class IDescriptorSetBinder;
class IPipelineDescriptorSetBinder;

struct DynamicOffsetDescriptors {
    BASE_NS::array_view<const RenderHandle> resources;
};

/**
class NodeContextDescriptorSetManager.
*/
class NodeContextDescriptorSetManager : public INodeContextDescriptorSetManager {
public:
    ~NodeContextDescriptorSetManager() override = default;

    static constexpr inline bool IsDynamicDescriptor(const DescriptorType descType)
    {
        return ((descType == DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                (descType == DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC));
    }

    void ResetAndReserve(const DescriptorCounts& descriptorCounts) override;
    void ResetAndReserve(const BASE_NS::array_view<DescriptorCounts> descriptorCounts) override;

    virtual RenderHandle CreateDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override = 0;
    BASE_NS::vector<RenderHandle> CreateDescriptorSets(
        const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings) override;
    RenderHandle CreateDescriptorSet(const uint32_t set, const PipelineLayout& pipelineLayout) override;

    IDescriptorSetBinder::Ptr CreateDescriptorSetBinder(const RenderHandle handle,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;

    IPipelineDescriptorSetBinder::Ptr CreatePipelineDescriptorSetBinder(const PipelineLayout& pipelineLayout) override;
    IPipelineDescriptorSetBinder::Ptr CreatePipelineDescriptorSetBinder(const PipelineLayout& pipelineLayout,
        const BASE_NS::array_view<const RenderHandle> handles,
        const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings) override;

    virtual RenderHandle CreateOneFrameDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override = 0;
    virtual BASE_NS::vector<RenderHandle> CreateOneFrameDescriptorSets(
        const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings) override;
    virtual RenderHandle CreateOneFrameDescriptorSet(const uint32_t set, const PipelineLayout& pipelineLayout) override;

    DescriptorSetLayoutBindingResources GetCpuDescriptorSetData(const RenderHandle handle) const;
    DynamicOffsetDescriptors GetDynamicOffsetDescriptors(const RenderHandle handle) const;

    virtual void BeginFrame();

    // information for barrier creation what kind of descriptors does the descriptor set contain
    // if returns false -> no need for barriers and/or layout changes
    bool HasDynamicBarrierResources(const RenderHandle handle) const;
    // count of dynamic offset needed when binding descriptor set
    uint32_t GetDynamicOffsetDescriptorCount(const RenderHandle handle) const;

    // information that this descriptor has a special platform hwbuffer resources bound
    // they might need some special platform specific handling
    bool HasPlatformBufferBindings(const RenderHandle handle) const;

    // update descriptor sets for cpu data (adds correct gpu queue as well)
    bool UpdateCpuDescriptorSet(const RenderHandle handle, const DescriptorSetLayoutBindingResources& bindingResources,
        const GpuQueue& gpuQueue);
    // call from backend before actual graphics api updateDescriptorset()
    // advances the gpu handle to the next available descriptor set (ring buffer)
    virtual void UpdateDescriptorSetGpuHandle(const RenderHandle handle) = 0;
    // platform specific updates
    virtual void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) = 0;

    struct CpuDescriptorSet {
        uint32_t currentGpuBufferingIndex { 0 };
        bool gpuDescriptorSetCreated { false };
        bool isDirty { false };
        bool hasDynamicBarrierResources { false };
        bool hasPlatformConversionBindings { false }; // e.g. hwbuffers with ycbcr / OES
        bool hasImmutableSamplers { false };

        BASE_NS::vector<DescriptorSetLayoutBindingResource> bindings;

        BASE_NS::vector<BufferDescriptor> buffers;
        BASE_NS::vector<ImageDescriptor> images;
        BASE_NS::vector<SamplerDescriptor> samplers;

        // gpu buffers with dynamic offsets
        BASE_NS::vector<RenderHandle> dynamicOffsetDescriptors;
    };

#if (RENDER_VALIDATION_ENABLED == 1)
    struct DescriptorCountValidation {
        BASE_NS::unordered_map<uint32_t, int32_t> typeToCount;
    };
#endif
#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
    void SetValidationDebugName(const BASE_NS::string_view debugName);
#endif
protected:
    static constexpr uint32_t ONE_FRAME_DESC_SET_BIT { 1u };
    enum DescriptorSetIndexType : uint8_t {
        DESCRIPTOR_SET_INDEX_TYPE_STATIC = 0,
        DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME,
        DESCRIPTOR_SET_INDEX_TYPE_COUNT,
    };

    NodeContextDescriptorSetManager() = default;
    BASE_NS::vector<CpuDescriptorSet> cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_COUNT];

    uint32_t maxSets_ { 0 };
    // indicates if there are some sets updated on CPU which have platfrom conversion bindings
    bool hasPlatformConversionBindings_ { false };

    bool UpdateCpuDescriptorSetImpl(const uint32_t index, const DescriptorSetLayoutBindingResources& bindingResources,
        const GpuQueue& gpuQueue, BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSets);
    DescriptorSetLayoutBindingResources GetCpuDescriptorSetDataImpl(
        const uint32_t index, const BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSet) const;
    DynamicOffsetDescriptors GetDynamicOffsetDescriptorsImpl(
        const uint32_t index, const BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSet) const;
    bool HasDynamicBarrierResourcesImpl(
        const uint32_t index, const BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSet) const;
    uint32_t GetDynamicOffsetDescriptorCountImpl(
        const uint32_t index, const BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSet) const;
    bool HasPlatformBufferBindingsImpl(
        const uint32_t index, const BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSet) const;

#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
    BASE_NS::string debugName_;
#endif
private:
#if (RENDER_VALIDATION_ENABLED == 1)
    DescriptorCountValidation descriptorCountValidation_;
#endif
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_H
