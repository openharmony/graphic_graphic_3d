/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MALEOON_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_MLN_H
#define MALEOON_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_MLN_H

#include <cstdint>

#include <base/containers/vector.h>
#include <render/namespace.h>

#include "nodecontext/node_context_descriptor_set_manager.h"
#include "default_engine_constants.h"

#include <maleoon/maleoon.h>

RENDER_BEGIN_NAMESPACE()

class Device;
class GpuResourceManager;

struct LowLevelDescriptorSetMln {
    MlnBindingLayout bindingLayout{MLN_NULL_HANDLE};
    static constexpr uint32_t MAX_BUFFERING_COUNT{DeviceConstants::MAX_BUFFERING_COUNT};
    MlnBindingSet bufferingSet[MAX_BUFFERING_COUNT]{};
    // Ensure all buffering sets receive initial writes: on creation, set to bufferingCount_.
    // Decremented each frame; forces UpdateDescriptorSetGpuHandle to return dirty=true
    // even after isDirty is cleared, so every buffering slot gets written at least once.
    uint32_t dirtyFramesRemaining{0};
};

class NodeContextDescriptorSetManagerMln final : public NodeContextDescriptorSetManager {
public:
    explicit NodeContextDescriptorSetManagerMln(Device& device);
    ~NodeContextDescriptorSetManagerMln() override;

    void ResetAndReserve(const DescriptorCounts& descriptorCounts) override;
    void BeginFrame() override;

    RenderHandle CreateDescriptorSet(
        BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;
    RenderHandle CreateOneFrameDescriptorSet(
        BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;

    bool UpdateDescriptorSetGpuHandle(const RenderHandle handle) override;
    void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) override;

    // Called by render backend before processing commands
    void BeginBackendFrame();

    // Resolve a descriptor set handle to its MlnBindingSet for the current buffering index
    MlnBindingSet GetMlnBindingSet(const RenderHandle handle) const;

    // Get the low-level descriptor set data (layout + sets)
    const LowLevelDescriptorSetMln* GetLowLevelDescriptorSet(const RenderHandle handle) const;

private:
    struct PendingOneFrameGpuData {
        uint64_t frameIndex{0};
        BASE_NS::vector<LowLevelDescriptorSetMln> gpuSets;
    };

    RenderHandle CreateDescriptorSetImpl(
        BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings,
        DescriptorSetIndexType indexType);

    void DestroyOneFrameGpuData();
    void DestroyGpuDescriptorSetArray(BASE_NS::vector<LowLevelDescriptorSetMln>& gpuSets);
    void ReclaimAgedOneFrameGpuData();

    uint32_t bufferingCount_{0};

    // GPU-side storage parallel to cpuDescriptorSets_
    BASE_NS::vector<LowLevelDescriptorSetMln> gpuDescriptorSets_[DESCRIPTOR_SET_INDEX_TYPE_COUNT];

    // One-frame descriptor sets must outlive in-flight GPU work.
    BASE_NS::vector<PendingOneFrameGpuData> pendingOneFrameGpuData_;
};

RENDER_END_NAMESPACE()

#endif  // MALEOON_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_MLN_H
