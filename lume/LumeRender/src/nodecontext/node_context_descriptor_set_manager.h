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

#ifndef RENDER_NODECONTEXT_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_H
#define RENDER_NODECONTEXT_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_H

#include <cstdint>
#include <shared_mutex>

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

struct BufferDescriptorHandler {
    BufferDescriptor desc;
    // fetched and compared for possibly needed changes
    EngineResourceHandle handle;
};

struct ImageDescriptorHandler {
    ImageDescriptor desc;
    // fetched and compared for possibly needed changes
    EngineResourceHandle handle;
    EngineResourceHandle samplerHandle;
};

struct SamplerDescriptorHandler {
    SamplerDescriptor desc;
    // fetched and compared for possibly needed changes
    EngineResourceHandle handle;
};

/** Descriptor set layout binding resources */
struct DescriptorSetLayoutBindingResourcesHandler {
    /** Bindings */
    BASE_NS::array_view<const DescriptorSetLayoutBindingResource> bindings;

    /** Buffer descriptors */
    BASE_NS::array_view<const BufferDescriptorHandler> buffers;
    /** Image descriptors */
    BASE_NS::array_view<const ImageDescriptorHandler> images;
    /** Sampler descriptors */
    BASE_NS::array_view<const SamplerDescriptorHandler> samplers;

    /** Mask of bindings in the descriptor set. Max uint is value which means that not set */
    uint32_t descriptorSetBindingMask { ~0u };
    /** Current binding mask. Max uint is value which means that not set */
    uint32_t bindingMask { ~0u };
};

struct CpuDescriptorSet {
    uint32_t currentGpuBufferingIndex { 0 };
    bool gpuDescriptorSetCreated { false };
    bool isDirty { false };
    bool hasDynamicBarrierResources { false };
    bool hasPlatformConversionBindings { false }; // e.g. hwbuffers with ycbcr / OES
    bool hasImmutableSamplers { false };

    BASE_NS::vector<DescriptorSetLayoutBindingResource> bindings;

    BASE_NS::vector<BufferDescriptorHandler> buffers;
    BASE_NS::vector<ImageDescriptorHandler> images;
    BASE_NS::vector<SamplerDescriptorHandler> samplers;

    // gpu buffers with dynamic offsets
    BASE_NS::vector<RenderHandle> dynamicOffsetDescriptors;
};

// storage counts
struct LowLevelDescriptorCounts {
    uint32_t writeDescriptorCount { 0u };
    uint32_t bufferCount { 0u };
    uint32_t imageCount { 0u };
    uint32_t samplerCount { 0u };
    uint32_t accelCount { 0u };
};

enum DescriptorSetUpdateInfoFlagBits {
    // invalid handles etc.
    DESCRIPTOR_SET_UPDATE_INFO_INVALID_BIT = (1 << 0),
    // new handles -> needs real update
    DESCRIPTOR_SET_UPDATE_INFO_NEW_BIT = (1 << 1),
};
using DescriptorSetUpdateInfoFlags = uint32_t;

/**
 * Global descriptor set manager
 * NOTE: The global descriptor sets are still updated through different NodeContextDescriptorSetManager s
 */
class DescriptorSetManager {
public:
    explicit DescriptorSetManager(Device& device);
    virtual ~DescriptorSetManager() = default;

    virtual void BeginFrame();

    void LockFrameCreation();

    BASE_NS::vector<RenderHandleReference> CreateDescriptorSets(const BASE_NS::string_view name,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings, const uint32_t count);
    RenderHandleReference CreateDescriptorSet(const BASE_NS::string_view name,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings);

    // get one
    RenderHandle GetDescriptorSetHandle(const BASE_NS::string_view name) const;
    // get all descriptor sets defined with this name
    BASE_NS::array_view<const RenderHandle> GetDescriptorSetHandles(const BASE_NS::string_view name) const;

    CpuDescriptorSet* GetCpuDescriptorSet(const RenderHandle& handle) const;
    DescriptorSetLayoutBindingResourcesHandler GetCpuDescriptorSetData(const RenderHandle& handle) const;
    DynamicOffsetDescriptors GetDynamicOffsetDescriptors(const RenderHandle& handle) const;
    bool HasDynamicBarrierResources(const RenderHandle& handle) const;
    uint32_t GetDynamicOffsetDescriptorCount(const RenderHandle& handle) const;

    bool HasPlatformConversionBindings(const RenderHandle& handle) const;

    // Will mark the descriptor set write locked for this frame (no one else can write)
    DescriptorSetUpdateInfoFlags UpdateCpuDescriptorSet(const RenderHandle& handle,
        const DescriptorSetLayoutBindingResources& bindingResources, const GpuQueue& gpuQueue);
    // platform
    virtual bool UpdateDescriptorSetGpuHandle(const RenderHandle& handle) = 0;
    virtual void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) = 0;

    // get all updated handles this frame
    BASE_NS::array_view<const RenderHandle> GetUpdateDescriptorSetHandles() const;

    class LowLevelDescriptorSetData {
    public:
        LowLevelDescriptorSetData() = default;
        virtual ~LowLevelDescriptorSetData() = default;

        LowLevelDescriptorSetData(const LowLevelDescriptorSetData&) = delete;
        LowLevelDescriptorSetData& operator=(const LowLevelDescriptorSetData&) = delete;
    };

    struct GlobalDescriptorSetData {
        // should only be updated from a single place per frame
        bool frameWriteLocked { false };
        CpuDescriptorSet cpuDescriptorSet;
        RenderHandleReference renderHandleReference;
    };
    struct GlobalDescriptorSetBase {
        // handles are separated for direct array_view usage
        BASE_NS::vector<RenderHandle> handles;
        BASE_NS::vector<GlobalDescriptorSetData> data;
        BASE_NS::string name;
    };

protected:
    // platform
    virtual void CreateDescriptorSets(const uint32_t arrayIndex, const uint32_t descriptorSetCount,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) = 0;

    Device& device_;

    // NOTE: does not need mutuxes at the moment
    // creation happens only in render nodes init or preexecute
    BASE_NS::vector<BASE_NS::unique_ptr<GlobalDescriptorSetBase>> descriptorSets_;
    BASE_NS::unordered_map<BASE_NS::string, uint32_t> nameToIndex_;

    BASE_NS::vector<RenderHandle> availableHandles_;

    // all handles this frame which should be updated (needs to be locked when accessed)
    BASE_NS::vector<RenderHandle> descriptorSetHandlesForUpdate_;

    // lock creation after pre execute done for render nodes
    bool creationLocked_ { true };

    // used to lock shared data which could be accessed from multiple threads
    mutable std::shared_mutex mutex_;
};

/**
class NodeContextDescriptorSetManager.
*/
class NodeContextDescriptorSetManager : public INodeContextDescriptorSetManager {
public:
    explicit NodeContextDescriptorSetManager(Device& device);
    ~NodeContextDescriptorSetManager() override = default;

    static constexpr inline bool IsDynamicDescriptor(const DescriptorType descType)
    {
        return ((descType == DescriptorType::CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) ||
                (descType == DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC));
    }

    static constexpr inline uint32_t GetCpuDescriptorSetIndex(const RenderHandle& handle)
    {
        const uint32_t addBits = RenderHandleUtil::GetAdditionalData(handle);
        if (addBits & NodeContextDescriptorSetManager::ONE_FRAME_DESC_SET_BIT) {
            return NodeContextDescriptorSetManager::DescriptorSetIndexType::DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME;
        } else if (addBits & NodeContextDescriptorSetManager::GLOBAL_DESCRIPTOR_BIT) {
            return ~0U; // invalid
        } else {
            return NodeContextDescriptorSetManager::DescriptorSetIndexType::DESCRIPTOR_SET_INDEX_TYPE_STATIC;
        }
    }

    // static helper function
    static void IncreaseDescriptorSetCounts(const DescriptorSetLayoutBinding& refBinding,
        LowLevelDescriptorCounts& descSetCounts, uint32_t& dynamicOffsetCount);

    void ResetAndReserve(const DescriptorCounts& descriptorCounts) override;
    void ResetAndReserve(const BASE_NS::array_view<DescriptorCounts> descriptorCounts) override;

    virtual RenderHandle CreateDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override = 0;
    BASE_NS::vector<RenderHandle> CreateDescriptorSets(
        const BASE_NS::array_view<const DescriptorSetLayoutBindings> descriptorSetsLayoutBindings) override;
    RenderHandle CreateDescriptorSet(const uint32_t set, const PipelineLayout& pipelineLayout) override;

    IDescriptorSetBinder::Ptr CreateDescriptorSetBinder(const RenderHandle handle,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;
    IDescriptorSetBinder::Ptr CreateDescriptorSetBinder(
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

    RenderHandleReference CreateGlobalDescriptorSet(const BASE_NS::string_view name,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;
    BASE_NS::vector<RenderHandleReference> CreateGlobalDescriptorSets(const BASE_NS::string_view name,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings,
        const uint32_t descriptorSetCount) override;
    RenderHandle GetGlobalDescriptorSet(const BASE_NS::string_view name) const override;
    BASE_NS::array_view<const RenderHandle> GetGlobalDescriptorSets(const BASE_NS::string_view name) const override;

    DescriptorSetLayoutBindingResourcesHandler GetCpuDescriptorSetData(const RenderHandle handle) const;
    DynamicOffsetDescriptors GetDynamicOffsetDescriptors(const RenderHandle handle) const;

    virtual void BeginFrame();

    // information for barrier creation what kind of descriptors does the descriptor set contain
    // if returns false -> no need for barriers and/or layout changes
    bool HasDynamicBarrierResources(const RenderHandle handle) const;
    // count of dynamic offset needed when binding descriptor set
    uint32_t GetDynamicOffsetDescriptorCount(const RenderHandle handle) const;

    bool HasPlatformConversionBindings(const RenderHandle handle) const;

    // update descriptor sets for cpu data (adds correct gpu queue as well)
    DescriptorSetUpdateInfoFlags UpdateCpuDescriptorSet(const RenderHandle handle,
        const DescriptorSetLayoutBindingResources& bindingResources, const GpuQueue& gpuQueue);
    // call from backend before actual graphics api updateDescriptorset()
    // advances the gpu handle to the next available descriptor set (ring buffer)
    // if returns false, then the update should not be handled in the backend
    virtual bool UpdateDescriptorSetGpuHandle(const RenderHandle handle) = 0;
    // platform specific updates
    virtual void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) = 0;

#if (RENDER_VALIDATION_ENABLED == 1)
    struct DescriptorCountValidation {
        BASE_NS::unordered_map<uint32_t, int32_t> typeToCount;
    };
#endif
#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
    void SetValidationDebugName(const BASE_NS::string_view debugName);
#endif

    static constexpr uint32_t ONE_FRAME_DESC_SET_BIT { 1U << 0U };
    static constexpr uint32_t GLOBAL_DESCRIPTOR_BIT { 1U << 1U };

    enum DescriptorSetIndexType : uint8_t {
        DESCRIPTOR_SET_INDEX_TYPE_STATIC = 0,
        DESCRIPTOR_SET_INDEX_TYPE_ONE_FRAME,
        DESCRIPTOR_SET_INDEX_TYPE_COUNT,
    };

protected:
    NodeContextDescriptorSetManager() = delete;
    Device& device_;

    BASE_NS::vector<CpuDescriptorSet> cpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_COUNT];

    uint32_t maxSets_ { 0 };
    // indicates if there are some sets updated on CPU which have platfrom conversion bindings
    bool hasPlatformConversionBindings_ { false };

    DescriptorSetUpdateInfoFlags UpdateCpuDescriptorSetImpl(const uint32_t index,
        const DescriptorSetLayoutBindingResources& bindingResources, const GpuQueue& gpuQueue,
        BASE_NS::array_view<CpuDescriptorSet> cpuDescriptorSets);
    static DescriptorSetLayoutBindingResourcesHandler GetCpuDescriptorSetDataImpl(
        const uint32_t index, const BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSet);
    static DynamicOffsetDescriptors GetDynamicOffsetDescriptorsImpl(
        const uint32_t index, const BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSet);
    static bool HasDynamicBarrierResourcesImpl(
        const uint32_t index, const BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSet);
    static uint32_t GetDynamicOffsetDescriptorCountImpl(
        const uint32_t index, const BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSet);
    static bool HasPlatformConversionBindingsImpl(
        const uint32_t index, const BASE_NS::vector<CpuDescriptorSet>& cpuDescriptorSet);

    BASE_NS::string debugName_;
    DescriptorSetManager& globalDescriptorSetMgr_;

private:
#if (RENDER_VALIDATION_ENABLED == 1)
    DescriptorCountValidation descriptorCountValidation_;
#endif
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_H
