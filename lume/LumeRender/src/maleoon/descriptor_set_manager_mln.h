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

#ifndef MALEOON_DESCRIPTOR_SET_MANAGER_MLN_H
#define MALEOON_DESCRIPTOR_SET_MANAGER_MLN_H

#include <base/containers/vector.h>
#include <render/namespace.h>

#include "nodecontext/node_context_descriptor_set_manager.h"
#include "default_engine_constants.h"

#include <maleoon/maleoon.h>

RENDER_BEGIN_NAMESPACE()

class Device;

struct LowLevelGlobalDescriptorSetMln {
    MlnBindingLayout bindingLayout { MLN_NULL_HANDLE };
    static constexpr uint32_t MAX_BUFFERING_COUNT { DeviceConstants::MAX_BUFFERING_COUNT };
    MlnBindingSet bufferingSet[MAX_BUFFERING_COUNT] {};
    // Ensure all buffering sets receive initial writes.
    uint32_t dirtyFramesRemaining { 0 };
};

/**
 * Global descriptor set manager for Maleoon backend.
 * Manages descriptor sets that persist across render node contexts.
 */
class DescriptorSetManagerMln final : public DescriptorSetManager {
public:
    explicit DescriptorSetManagerMln(Device& device);
    ~DescriptorSetManagerMln() override;

    void BeginFrame() override;
    void BeginBackendFrame();

    bool UpdateDescriptorSetGpuHandle(const RenderHandle& handle) override;
    void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) override;

    void CreateDescriptorSets(const uint32_t arrayIndex, const uint32_t descriptorSetCount,
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;

    const LowLevelGlobalDescriptorSetMln* GetDescriptorSet(const RenderHandle& handle) const;
    MlnBindingSet GetMlnBindingSet(const RenderHandle& handle) const;

private:
    MlnDevice mlnDevice_ { MLN_NULL_HANDLE };
    uint32_t bufferingCount_ { 0 };

    // GPU-side storage parallel to descriptorSets_ in base class
    BASE_NS::vector<BASE_NS::vector<LowLevelGlobalDescriptorSetMln>> gpuDescriptorSets_;
};

RENDER_END_NAMESPACE()

#endif // MALEOON_DESCRIPTOR_SET_MANAGER_MLN_H
