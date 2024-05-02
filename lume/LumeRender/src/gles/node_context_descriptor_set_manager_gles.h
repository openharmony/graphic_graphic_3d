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

#ifndef GLES_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_GLES_H
#define GLES_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_GLES_H

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>
#include <render/render_data_structures.h>

#include "nodecontext/node_context_descriptor_set_manager.h"

RENDER_BEGIN_NAMESPACE()
struct RenderPassBeginInfo;
class GpuResourceManager;

class NodeContextDescriptorSetManagerGLES final : public NodeContextDescriptorSetManager {
public:
    explicit NodeContextDescriptorSetManagerGLES(Device& device);
    ~NodeContextDescriptorSetManagerGLES();

    void ResetAndReserve(const DescriptorCounts& descriptorCounts) override;
    void BeginFrame() override;

    RenderHandle CreateDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;
    RenderHandle CreateOneFrameDescriptorSet(
        const BASE_NS::array_view<const DescriptorSetLayoutBinding> descriptorSetLayoutBindings) override;

    void UpdateDescriptorSetGpuHandle(const RenderHandle handle) override;
    void UpdateCpuDescriptorSetPlatform(const DescriptorSetLayoutBindingResources& bindingResources) override;

private:
    Device& device_;

    uint32_t oneFrameDescSetGeneration_ { 0u };
#if (RENDER_VALIDATION_ENABLED == 1)
    static constexpr uint32_t MAX_ONE_FRAME_GENERATION_IDX { 16u };
#endif
};
RENDER_END_NAMESPACE()

#endif // GLES_NODE_CONTEXT_DESCRIPTOR_SET_MANAGER_GLES_H
